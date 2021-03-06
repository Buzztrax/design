/* test handling looping in gstreamer
 *
 * gcc -g loop1.c -o loop1 `pkg-config gstreamer-1.0 gstreamer-controller-1.0 --cflags --libs`
 */

#include <stdio.h>
#include <stdlib.h>

#include <gst/gst.h>
#include <gst/controller/gstinterpolationcontrolsource.h>
#include <gst/controller/gstdirectcontrolbinding.h>

//#define SINK_NAME "alsasink"
#define SINK_NAME "pulsesink"
#define SRC_NAME "audiotestsrc"

#define GST_CAT_DEFAULT gst_test_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static GMainLoop *main_loop = NULL;
static GstEvent *play_seek_event = NULL;
static GstEvent *loop_seek_event = NULL;
// command line options
static gint num_loops = 10;
static gboolean flushing_loops = FALSE;

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

static void
state_changed (const GstBus * const bus, GstMessage * message, GstElement * bin)
{
  if (GST_MESSAGE_SRC (message) == GST_OBJECT (bin)) {
    GstStateChangeReturn res;
    GstState oldstate, newstate;

    gst_message_parse_state_changed (message, &oldstate, &newstate, NULL);
    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS ((GstBin *) bin, GST_DEBUG_GRAPH_SHOW_ALL,
        "loop1");
    GST_INFO ("state change on the bin: %s -> %s",
        gst_element_state_get_name (oldstate),
        gst_element_state_get_name (newstate));
    switch (GST_STATE_TRANSITION (oldstate, newstate)) {
      case GST_STATE_CHANGE_NULL_TO_READY:
        GST_INFO
            ("initial seek ===========================================================");
        send_initial_seek ((GstBin *) bin);
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
  static gint loop = 0;
  GstEvent *event =
      gst_event_copy (flushing_loops ? play_seek_event : loop_seek_event);

  GST_INFO
      ("loop playback (%2d) =====================================================",
      loop);
  gst_event_set_seqnum (event, gst_util_seqnum_next ());
  if (!(gst_element_send_event (bin, event))) {
    fprintf (stderr, "element failed to handle continuing play seek event\n");
    g_main_loop_quit (main_loop);
  } else {
    if (loop == num_loops) {
      g_main_loop_quit (main_loop);
    }
    loop++;
  }
}

static void
double_ctrl_value_set (GstControlSource * cs, GstClockTime t, gdouble v)
{
  gst_timed_value_control_source_set ((GstTimedValueControlSource *) cs,
      t * GST_MSECOND, v);
}


gint
main (gint argc, gchar ** argv)
{
  GstElement *bin, *src[2], *mix, *sink;
  GstControlSource *cs;
  GstBus *bus;
  GstStateChangeReturn res;

  /* init gstreamer */
  gst_init (&argc, &argv);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING);
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "loop", 0, "loop test");

  if (argc > 1) {
    num_loops = atoi (argv[1]);
    if (argc > 2) {
      flushing_loops = (atoi (argv[2]) != 0);
    }
  }

  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");
  /* see if we get errors */
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK (message_received), bin);
  g_signal_connect (bus, "message::warning", G_CALLBACK (message_received),
      bin);
  g_signal_connect (bus, "message::eos", G_CALLBACK (message_received), bin);
  g_signal_connect (bus, "message::segment-done", G_CALLBACK (segment_done),
      bin);
  g_signal_connect (bus, "message::async-done", G_CALLBACK (async_done), bin);
  g_signal_connect (bus, "message::state-changed", G_CALLBACK (state_changed),
      bin);
  gst_object_unref (bus);

  main_loop = g_main_loop_new (NULL, FALSE);

  /* make elements and add them to the bin */
  if (!(src[0] = gst_element_factory_make (SRC_NAME, "src1"))) {
    fprintf (stderr, "Can't create element \"" SRC_NAME "\"\n");
    exit (-1);
  }
  g_object_set (src[0], "wave", 2, "freq", 110.0, NULL);
  if (!(src[1] = gst_element_factory_make (SRC_NAME, "src2"))) {
    fprintf (stderr, "Can't create element \"" SRC_NAME "\"\n");
    exit (-1);
  }
  g_object_set (src[1], "wave", 3, "freq", 440.0, NULL);
  if (!(mix = gst_element_factory_make ("adder", "mix"))) {
    fprintf (stderr, "Can't create element \"adder\"\n");
    exit (-1);
  }
  if (!(sink = gst_element_factory_make (SINK_NAME, "sink"))) {
    fprintf (stderr, "Can't create element \"" SINK_NAME "\"\n");
    exit (-1);
  }
  gst_bin_add_many (GST_BIN (bin), src[0], src[1], mix, sink, NULL);

  /* link elements */
  if (!gst_element_link_many (src[0], mix, sink, NULL)) {
    fprintf (stderr, "Can't link part1\n");
    exit (-1);
  }
  if (!gst_element_link_many (src[1], mix, NULL)) {
    fprintf (stderr, "Can't link part2\n");
    exit (-1);
  }

  /* setup controller */
  cs = gst_interpolation_control_source_new ();
  g_object_set (cs, "mode", GST_INTERPOLATION_MODE_LINEAR, NULL);
  gst_object_add_control_binding (GST_OBJECT_CAST (src[0]),
      gst_direct_control_binding_new (GST_OBJECT_CAST (src[0]), "volume", cs));
  /* set control values */
  double_ctrl_value_set (cs, 0, 1.0);
  double_ctrl_value_set (cs, 1000, 0.0);

  /* add a controller to the source */
  cs = gst_interpolation_control_source_new ();
  g_object_set (cs, "mode", GST_INTERPOLATION_MODE_LINEAR, NULL);
  gst_object_add_control_binding (GST_OBJECT_CAST (src[1]),
      gst_direct_control_binding_new (GST_OBJECT_CAST (src[1]), "volume", cs));
  /* set control values */
  double_ctrl_value_set (cs, 0, 0.0);
  double_ctrl_value_set (cs, 1000, 1.0);

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
  res = gst_element_set_state (bin, GST_STATE_READY);
  if (res == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "Can't go to ready\n");
    exit (-1);
  }
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
