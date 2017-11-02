/* test seemles looping in gstreamer
 *
 * In buzztrax the loops are not smooth. The code below tries to reproduce the
 * issue.
 *
 * Things we excluded:
 * - it does not seem to get worse if we use a lot of fx
 * - it does not matter wheter fx is any of volume,queue,adder
 * - it is not the low-latency setting on the sink
 * - it does not seem to be the bins
 * - it is not the cpu load
 *   - we tried to renice and also change to rt schduling (rt helps a bit)
 * - it is not the number of elements
 * - it is not caused by sending out copies of upstream events in adder, as we
 *   have just one upstream in this example
 * - it is not the delay between getting segment_done and sending the new seek,
 *   using sync-message:segment-done does not show any improvement
 *
 * Things we noticed:
 * - on the netbook the loops are smoother when using alsasink, compared to
 *   pulsesink (CPU load is simmilar and less than 100% in both cases)
 * - when the break happens we get this in the log:
 *   WARN           baseaudiosink gstbaseaudiosink.c:1374:gst_base_audio_sink_get_alignment:<player> Unexpected discontinuity in audio timestamps of +0:00:00.120000000, resyncing
 * - it is caused by "queue ! adder"
 *
 * Some facts:
 * - basesrc/sink only post segment-done messages from _loop() (pull mode)
 *   - both pause the task also
 * - we only run the source in loop(), sink uses chain()
 * - therefore sources post segment_done events (downstream, serialized)
 * - the messages get aggregated by the bin, once a _done message is received
 *   for each previously received _start, the bin send _done
 * - especially in deep pipelines this will happend before the data actually
 *   reached the sink and should give us enough time to react
 * - now when we send the new (non-flushing seek) from sink to sources, there
 *   might be still buffers traveling downstream, those should not be
 *   interrupted
 * - src can send segment-start message and new-segment-event right away when
 *   they handle the seek, as they have been paused anyway
 *
 * - audiobasesink implements the time-sync code, gst_audio_base_sink_get_times()
 *   returns (-1,-1) to bypass the sync code in basesink
 *   - this breaks qos (thats why it is not working even if we enable it)
 *
 * - we're already missing buffers in gst_base_sink_chain_unlocked() (which
 *   calls gst_audio_base_sink_render()):
 *   if we play a song in buzztrax and run the loop2.sh script, we can see it in
 *   the generated png


 * - it seems to be causes be the collectpads behaviour in adder:
 *   - gst_adder_collected() is called when all pads have data
 *   - events are not queued!
 *   - we send our synthetic 'new-segemnt' from _collected() to ensure we send
 *     it before the buffers it applies to
 *   - we also should send the 'segment-done' from _collected() after we pushed
 *     the last buffer of the segment
 *   - events:
 *     - when we get a seek on 'src', we forward this to all sink-pads
 *       and flag that we need to send a new-segment ourself
 *     - each upstream will send a new-segment event, we will drop those
 *       FIXME: only once we have new-segment from each upstream we should
 *       flag that we need to send the new-segment outself?
 *       - segment-events are supposed to be serialized
 *     - each upstream will send a segment-done, we drop those
 *  - issuess:
 *    - the _src_event() is not called from the same thread as _collected()
 *    - when we get a seek, we haven't seek'ed yet, there might be buffers in
 *      flight, we are definitely truncating segments when looping
 *    - FIXME: when we get the seek, me must only queue up the new segment and
 *      activate it from _collected()


 * TODO:
 * - add a parameter to repeat the FX1/FX2 pairs n-times to verify how many
 *   buffers source creating quickly to restart playing
 * - also test the effect of bin nesting depth
 * - the graphs produced by ./gsttr-tsplot.py show that source-elements have
 *   a gap between segment-done and segment - what are bins waiting for?
 *   - we have the time we get the last segment-done
 *   - we have the time the bin posts the segment-done message:
 *     gst_element_post_message
 *   - check when the app seeks again: gst_element_send_event
 *
 * Design:
 * - the main issue is that we wait until all sources have posted SEGMENT_DONE,
 *   the bin has matched each SEGMENT_START with a SEGMENT_DONE, posted a
 *   SEGMENT_DONE posted in turn until the pipleline got all SEGMENTS finished
 *   and posts the SEGMENT_DONE message, the app has received it and sent a new
 *   SEEK event
 * - there are two variants of segmented seeks that could be done with less
 *   application involvement: looping, playists. We can implement looping as a
 *   special playlist.
 * - proposal
 *   - the application sends a PLAYLIST event before sending the initial,
 *     flushing segmented seek
 *   - the playlist event is a list of of tupels (seek event, repeat-count)
 *     - seek events should be non flushing and must have a stop position except
 *       if it is the last entry and repeat-count is 1
 *     - repeat-count has a special value for INDEF which is only allowed if
 *       this is the final entry
 *   - all bins that have sources which are not bins store the playlist
 *   - when a bin gets SEGMENT_DONE from a child that is not a bin, it sends the
 *     next seek event from the playlist (updating repeat counts)
 *   - the application can still handle SEGMENT_DONE to track playlist progress
 * - open issues: we still need a flushing seek to kickstart playback, ideally
 *   we send the PLAYLIST in PAUSED and playback start from it
 *
 * gcc -g loop2.c -o loop2 `pkg-config gstreamer-1.0 gstreamer-controller-1.0 --cflags --libs`
 * loop2 <num-loops> <flushing> <sync-msg>
 *
 * GST_DEBUG_NO_COLOR=1 GST_DEBUG_FILE="loop2.log" GST_DEBUG="*:2,loop:4,bt-core:5,basesrc:5,*basesink:5" ./loop2 10
 * ./loop2.sh
 */

#include <stdio.h>
#include <stdlib.h>

#include <gst/gst.h>
#include <gst/controller/gstinterpolationcontrolsource.h>
#include <gst/controller/gstdirectcontrolbinding.h>

// some 'fx' variants
// works
//#define FX1_NAME "queue"
//#define FX2_NAME "identity"

// this makes the most problems
//#define FX1_NAME "queue"
//#define FX2_NAME "adder"

// weird dropouts if non-flusing (queue related?)
// requires this for non flushing seeks: https://bugzilla.gnome.org/show_bug.cgi?id=757563
#define FX1_NAME "queue"
#define FX2_NAME "audiomixer"

// works with the patches in https://bugzilla.gnome.org/show_bug.cgi?id=757563
//#define FX1_NAME "identity"
//#define FX2_NAME "audiomixer"

// works
//#define FX1_NAME "identity"
//#define FX2_NAME "adder"

#define SRC_NAME "audiotestsrc"
#define SINK_NAME "pulsesink"

#define BPM 120
#define TPB 4

// undef to try to seek in ready state
#define GST_BUG_733031

#define GST_CAT_DEFAULT gst_test_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);


static GMainLoop *main_loop = NULL;
static GstEvent *play_seek_event = NULL;
static GstEvent *loop_seek_event = NULL;
// command line options
static gint num_loops = 10;
static gboolean flushing_seeks = FALSE;
static gboolean sync_message = FALSE;

static void
message_received (GstBus * bus, GstMessage * message, GstPipeline * pipeline)
{
  const GstStructure *s;

  s = gst_message_get_structure (message);
  g_print ("message from \"%s\" (%s): ",
      GST_STR_NULL (GST_ELEMENT_NAME (GST_MESSAGE_SRC (message))),
      gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
  if (s) {
    gchar *sstr;

    sstr = gst_structure_to_string (s);
    puts (sstr);
    g_free (sstr);
  } else {
    puts ("no message details");
  }

  g_main_loop_quit (main_loop);
}

#ifndef GST_BUG_733031
static void
send_initial_seek (GstBin * bin)
{
  GstIterator *it = gst_bin_iterate_sources (bin);
  GstElement *e;
  gboolean done = FALSE;
  GValue item = { 0, };

  while (!done) {
    switch (gst_iterator_next (it, &item)) {
      case GST_ITERATOR_OK:
        e = GST_ELEMENT (g_value_get_object (&item));
        if (GST_IS_BIN (e)) {
          send_initial_seek ((GstBin *) e);
        } else {
          if (!(gst_element_send_event (e, gst_event_ref (play_seek_event)))) {
            fprintf (stderr, "element failed to handle seek event\n");
            g_main_loop_quit (main_loop);
          }
        }
        g_value_reset (&item);
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (it);
        break;
      case GST_ITERATOR_ERROR:
        GST_WARNING ("wrong parameter for iterator");
        done = TRUE;
        break;
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
    }
  }
  g_value_unset (&item);
  gst_iterator_free (it);
}
#endif

static void
state_changed (const GstBus * const bus, GstMessage * message, GstElement * bin)
{
  if (GST_MESSAGE_SRC (message) == GST_OBJECT (bin)) {
    GstStateChangeReturn res;
    GstState oldstate, newstate;

    gst_message_parse_state_changed (message, &oldstate, &newstate, NULL);
    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS ((GstBin *) bin, GST_DEBUG_GRAPH_SHOW_ALL,
        "loop2");
    GST_INFO ("state change on the bin: %s -> %s",
        gst_element_state_get_name (oldstate),
        gst_element_state_get_name (newstate));
    switch (GST_STATE_TRANSITION (oldstate, newstate)) {
      case GST_STATE_CHANGE_NULL_TO_READY:
#ifndef GST_BUG_733031
        send_initial_seek ((GstBin *) bin);
#endif
        break;
      case GST_STATE_CHANGE_READY_TO_PAUSED:
        GST_INFO
            ("initial seek ===========================================================");
#ifdef GST_BUG_733031
        if (!(gst_element_send_event (bin, gst_event_ref (play_seek_event)))) {
          GST_WARNING_OBJECT (bin, "element failed to handle seek event");
        }
#endif
        // start playback
        GST_INFO
            ("start playing ==========================================================");
        res = gst_element_set_state (bin, GST_STATE_PLAYING);
        if (res == GST_STATE_CHANGE_FAILURE) {
          fprintf (stderr, "can't go to playing state\n");
          g_main_loop_quit (main_loop);
        } else if (res == GST_STATE_CHANGE_ASYNC) {
          GST_INFO ("->PLAYING needs async wait");
        }
        break;
      default:
        break;
    }
  }
}

static void
async_done (const GstBus * const bus, const GstMessage * const message,
    GstElement * bin)
{
  GST_INFO
      ("async done ============================================================");
}

static void
segment_done (const GstBus * const bus, const GstMessage * const message,
    GstElement * bin)
{
  GstEvent *event =
      gst_event_copy (flushing_seeks ? play_seek_event : loop_seek_event);
#ifndef GST_DISABLE_GST_DEBUG
  GstFormat format;
  gint64 position;
  guint32 seek_seqnum = gst_message_get_seqnum ((GstMessage *) message);
  // check how regullar the SEGMENT DONE comes
  static GstClockTime last_ts = 0;
  GstClockTime this_ts = gst_util_get_timestamp ();
  GstClockTimeDiff ts_diff = last_ts ? (this_ts - last_ts) : 0;
  last_ts = this_ts;

  gst_message_parse_segment_done ((GstMessage *) message, &format, &position);
#endif

  GST_INFO
      ("received SEGMENT_DONE (%u) bus message: from %s, with fmt=%s, ts=%"
      GST_TIME_FORMAT " after %" GST_TIME_FORMAT, seek_seqnum,
      GST_OBJECT_NAME (GST_MESSAGE_SRC (message)), gst_format_get_name (format),
      GST_TIME_ARGS (position), GST_TIME_ARGS (ts_diff));

  static gint loop = 0;
  loop++;
  if (loop == num_loops) {
    g_main_loop_quit (main_loop);
  }

  GST_INFO
      ("loop playback (%2d) =====================================================",
      loop);
  gst_event_set_seqnum (event, gst_util_seqnum_next ());
  if (!(gst_element_send_event (bin, event))) {
    fprintf (stderr, "element failed to handle continuing play seek event\n");
    g_main_loop_quit (main_loop);
  }
}

static void
double_ctrl_value_set (GstControlSource * cs, GstClockTime t, gdouble v)
{
  gst_timed_value_control_source_set ((GstTimedValueControlSource *) cs,
      t * GST_MSECOND, v);
}

static GstElement *
make_src (void)
{
  GstElement *e;
  GstControlSource *cs;
  gint spb;

  if (!(e = gst_element_factory_make (SRC_NAME, NULL))) {
    return NULL;
  }
  spb = (60 * 44100) / (BPM * TPB);
  g_object_set (e, "wave", 2, "samplesperbuffer", spb, NULL);

  /* setup controller */
  cs = gst_interpolation_control_source_new ();
  g_object_set (cs, "mode", GST_INTERPOLATION_MODE_LINEAR, NULL);
  gst_object_add_control_binding (GST_OBJECT_CAST (e),
      gst_direct_control_binding_new (GST_OBJECT_CAST (e), "volume", cs));

  /* set control values */
  double_ctrl_value_set (cs, 0, 1.0);
  double_ctrl_value_set (cs, 249, 0.0);
  double_ctrl_value_set (cs, 250, 1.0);
  double_ctrl_value_set (cs, 449, 0.0);
  double_ctrl_value_set (cs, 500, 1.0);
  double_ctrl_value_set (cs, 749, 0.0);
  double_ctrl_value_set (cs, 750, 1.0);
  double_ctrl_value_set (cs, 999, 0.0);

  /* setup controller */
  cs = gst_interpolation_control_source_new ();
  g_object_set (cs, "mode", GST_INTERPOLATION_MODE_LINEAR, NULL);
  gst_object_add_control_binding (GST_OBJECT_CAST (e),
      gst_direct_control_binding_new_absolute (GST_OBJECT_CAST (e), "freq", cs));

  /* set control values */
  double_ctrl_value_set (cs, 0, 110.0);
  double_ctrl_value_set (cs, 999, 440.0);

  return e;
}

static GstElement *
make_fx (const gchar *element_name)
{
  GstElement *e;

  if (!(e = gst_element_factory_make (element_name, NULL))) {
    return NULL;
  }
  if (!strcmp (element_name, "queue")) {
    g_object_set (G_OBJECT (e), "max-size-buffers", 1, "max-size-bytes", 0,
        "max-size-time", G_GUINT64_CONSTANT (0), "silent", TRUE, NULL);
  }
  return e;
}

static GstElement *
make_sink (void)
{
  GstElement *e;
  gint64 chunk;

  if (!(e = gst_element_factory_make (SINK_NAME, NULL))) {
    return NULL;
  }
  chunk = GST_TIME_AS_USECONDS ((GST_SECOND * 60) / (BPM * TPB));
  GST_INFO ("changing audio chunk-size for sink to %" G_GUINT64_FORMAT
      " µs = %" G_GUINT64_FORMAT " ms", chunk,
      (chunk / G_GINT64_CONSTANT (1000)));
  g_object_set (e, "latency-time", chunk, "buffer-time", chunk << 1, NULL);

  return e;
}

gint
main (gint argc, gchar ** argv)
{
  GstElement *bin, *src, *fx1, *fx2, *sink;
  GstBus *bus;
  GstStateChangeReturn res;

  /* init gstreamer */
  gst_init (&argc, &argv);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING);
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "loop", 0, "loop test");

  if (argc > 1) {
    num_loops = atoi (argv[1]);
    if (argc > 2) {
      flushing_seeks = (atoi (argv[2]) != 0);
      if (argc > 3) {
        sync_message = (atoi (argv[3]) != 0);
      }
    }
  }

  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("pipeline");
  /* register bus message handlers */
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  if (sync_message) {
    gst_bus_enable_sync_message_emission (bus);
    g_signal_connect (bus, "sync-message::segment-done",
        G_CALLBACK (segment_done), bin);
  } else {
    g_signal_connect (bus, "message::segment-done", G_CALLBACK (segment_done),
        bin);
  }
  g_signal_connect (bus, "message::error", G_CALLBACK (message_received), bin);
  g_signal_connect (bus, "message::warning", G_CALLBACK (message_received),
      bin);
  g_signal_connect (bus, "message::eos", G_CALLBACK (message_received), bin);
  g_signal_connect (bus, "message::async-done", G_CALLBACK (async_done), bin);
  g_signal_connect (bus, "message::state-changed", G_CALLBACK (state_changed),
      bin);
  gst_object_unref (G_OBJECT (bus));

  main_loop = g_main_loop_new (NULL, FALSE);

  /* make elements and add them to the bin */
  if (!(src = make_src ())) {
    fprintf (stderr, "Can't create element \"" SRC_NAME "\"\n");
    exit (-1);
  }
  if (!(fx1 = make_fx (FX1_NAME))) {
    fprintf (stderr, "Can't create element \"" FX1_NAME "\"\n");
    exit (-1);
  }
  if (!(fx2 = make_fx (FX2_NAME))) {
    fprintf (stderr, "Can't create element \"" FX2_NAME "\"\n");
    exit (-1);
  }
  if (!(sink = make_sink ())) {
    fprintf (stderr, "Can't create element \"" SINK_NAME "\"\n");
    exit (-1);
  }
  gst_bin_add_many (GST_BIN (bin), src, fx1, fx2, sink, NULL);

  /* link elements */
  if (!gst_element_link_many (src, fx1, fx2, sink, NULL)) {
    fprintf (stderr, "Can't link elements\n");
    exit (-1);
  }

  /* initial seek event */
  play_seek_event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
      GST_SEEK_TYPE_SET, (GstClockTime) 0,
      GST_SEEK_TYPE_SET, (GstClockTime) GST_SECOND);
  /* loop seek event (without flush) */
  loop_seek_event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_SEGMENT,
      GST_SEEK_TYPE_SET, (GstClockTime) 0,
      GST_SEEK_TYPE_SET, (GstClockTime) GST_SECOND);

  /* prepare playing */
  GST_INFO
      ("prepare playing ========================================================");
#ifdef GST_BUG_733031
  res = gst_element_set_state (bin, GST_STATE_PAUSED);
  if (res == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "Can't go to paused\n");
    exit (-1);
  }
#else
  res = gst_element_set_state (bin, GST_STATE_READY);
  if (res == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "Can't go to ready\n");
    exit (-1);
  }
#endif
  g_main_loop_run (main_loop);

  /* stop the pipeline */
  GST_INFO
      ("exiting ================================================================");
  gst_element_set_state (bin, GST_STATE_NULL);

  gst_event_unref (play_seek_event);
  gst_event_unref (loop_seek_event);
  gst_object_unref (GST_OBJECT (bin));
  g_main_loop_unref (main_loop);
  exit (0);
}
