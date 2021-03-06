/* test mute, solo, bypass stuff in gst
 * - just keep muted elements outgoing pad blocked
 *
 * gcc -Wall -g states1b.c -o states1b`pkg-config gstreamer-0.10 --cflags --libs`
 */

#include <stdio.h>
#include <gst/gst.h>

#define SINK_NAME "alsasink"
//#define SINK_NAME "esdsink"
#define ELEM_NAME "audioconvert"
#define SRC_NAME "audiotestsrc"
#define SILENCE_NAME "audiotestsrc"

#define WAIT_LENGTH 2

static gint64
query_and_print (GstElement * element, GstQuery * query)
{
  gint64 pos = 0;

  if (gst_element_query (element, query)) {
    gst_query_parse_position (query, NULL, &pos);
    printf ("%s playback-pos : cur=%" G_GINT64_FORMAT "\n",
        GST_OBJECT_NAME (element), pos);
  } else {
    printf ("%s playback-pos : cur=???\n", GST_OBJECT_NAME (element));
  }
  return (pos);
}

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
    printf ("%s\n", sstr);
    g_free (sstr);
  } else {
    printf ("no message details\n");
  }
}

int
main (int argc, char **argv)
{
  GstElement *bin;
  /* elements used in pipeline */
  GstElement *sink;
  GstElement *src1, *src2;
  GstElement *mix;
  GstClock *clock;
  GstClockID clock_id;
  GstClockReturn wait_ret;
  GstQuery *query;
  GstPad *src1_src;
  GstPad *src2_src;
  GstBus *bus;
  gint64 pos;
  GstEvent *seek_event;

  /* init gstreamer */
  gst_init (&argc, &argv);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING);

  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");
  clock = gst_pipeline_get_clock (GST_PIPELINE (bin));

  /* make a sink */
  if (!(sink = gst_element_factory_make (SINK_NAME, "sink"))) {
    fprintf (stderr, "Can't create element \"" SINK_NAME "\"\n");
    exit (-1);
  }

  /* make sources */
  if (!(src1 = gst_element_factory_make (SRC_NAME, "src1"))) {
    fprintf (stderr, "Can't create element \"" SRC_NAME "\"\n");
    exit (-1);
  }
  g_object_set (src1, "freq", 440.0, NULL);
  if (!(src2 = gst_element_factory_make (SRC_NAME, "src2"))) {
    fprintf (stderr, "Can't create element \"" SRC_NAME "\"\n");
    exit (-1);
  }
  g_object_set (src2, "freq", 2000.0, NULL);

  /* make a mixer */
  if (!(mix = gst_element_factory_make ("adder", "mix"))) {
    fprintf (stderr, "Can't create element \"adder\"\n");
    exit (-1);
  }

  /* add objects to the main bin */
  gst_bin_add_many (GST_BIN (bin), src1, src2, mix, sink, NULL);

  /* link elements */
  if (!gst_element_link_many (src1, mix, sink, NULL)) {
    fprintf (stderr, "Can't link first part\n");
    exit (-1);
  }
  if (!gst_element_link_many (src2, mix, NULL)) {
    fprintf (stderr, "Can't link second part\n");
    exit (-1);
  }

  /* make a position query */
  if (!(query = gst_query_new_position (GST_FORMAT_TIME))) {
    fprintf (stderr, "Can't make a position query\n");
    exit (-1);
  }

  /* see if we get errors */
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK (message_received), bin);
  g_signal_connect (bus, "message::warning", G_CALLBACK (message_received),
      bin);

  /* prepare playing */
  if (gst_element_set_state (bin, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "Can't prepare playing\n");
    exit (-1);
  }

  /* get pads */
  if (!(src1_src = gst_element_get_pad (src1, "src"))) {
    fprintf (stderr, "Can't get src pad of src1\n");
    exit (-1);
  }
  if (!(src2_src = gst_element_get_pad (src2, "src"))) {
    fprintf (stderr, "Can't get src pad of src1\n");
    exit (-1);
  }

  clock_id = gst_clock_new_periodic_id (clock,
      gst_clock_get_time (clock) + (WAIT_LENGTH * GST_SECOND),
      (WAIT_LENGTH * GST_SECOND));

  /* start playing */
  if (gst_element_set_state (bin,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "Can't start playing\n");
    exit (-1);
  }

  /* do the state tests */

  puts ("play everything ========================================================\n");
  query_and_print (src1, query);
  query_and_print (src2, query);
  if ((wait_ret = gst_clock_id_wait (clock_id, NULL)) != GST_CLOCK_OK) {
    GST_WARNING ("clock_id_wait returned: %d", wait_ret);
  }

  puts ("trying to pause source2 ================================================\n");
  if (!gst_pad_set_blocked (src2_src, TRUE)) {
    fprintf (stderr, "can't block src-pad of src2");
  }
  if (gst_element_set_state (src2,
          GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "Can't set state to READY for src2\n");
    exit (-1);
  }
  if (!gst_element_set_locked_state (src2, TRUE)) {
    fprintf (stderr, "Can't lock state of src2\n");
  }

  puts ("paused source2 =========================================================\n");
  query_and_print (src1, query);
  if ((wait_ret = gst_clock_id_wait (clock_id, NULL)) != GST_CLOCK_OK) {
    fprintf (stderr, "clock_id_wait returned: %d\n", wait_ret);
  }

  puts ("trying to continue source2 and pause source1 ===========================\n");
  pos = query_and_print (src1, query);
  seek_event = gst_event_new_seek (1.0, GST_FORMAT_DEFAULT,
      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
      GST_SEEK_TYPE_SET, (GstClockTime) (pos),
      GST_SEEK_TYPE_SET, (GstClockTime) (G_MAXUINT64 - 1));

  if (!(gst_element_send_event (src2, seek_event))) {
    GST_WARNING ("Can't send new-segment to src2");
  }
  if (!gst_element_set_locked_state (src2, FALSE)) {
    fprintf (stderr, "Can't unlock state of src2\n");
  }
  if (gst_element_set_state (src2,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "Can't set state to PLAYING src2\n");
    exit (-1);
  }
  // hack ?
  gst_pad_set_active (src2_src, FALSE);
  if (!gst_pad_set_active (src2_src, TRUE)) {
    fprintf (stderr, "can't activate src-pad of src2\n");
  }

  if (!gst_pad_set_blocked (src1_src, TRUE)) {
    fprintf (stderr, "can't block src-pad of src1\n");
  }
  if (gst_element_set_state (src1,
          GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "Can't set state to READY for src1\n");
    exit (-1);
  }
  if (!gst_element_set_locked_state (src1, TRUE)) {
    fprintf (stderr, "Can't lock state of src1\n");
  }

  puts ("continued source2, paused source1 ======================================\n");
  query_and_print (src1, query);
  query_and_print (src2, query);
  if ((wait_ret = gst_clock_id_wait (clock_id, NULL)) != GST_CLOCK_OK) {
    fprintf (stderr, "clock_id_wait returned: %d\n", wait_ret);
  }

  /* stop the pipeline */
  puts ("playing done ===========================================================\n");
  if (!gst_element_set_locked_state (src1, FALSE)) {
    fprintf (stderr, "Can't unlock state of src1\n");
  }
  gst_element_set_state (bin, GST_STATE_NULL);

  /* we don't need a reference to these objects anymore */
  gst_query_unref (query);
  gst_clock_id_unref (clock_id);
  gst_object_unref (GST_OBJECT (bus));
  gst_object_unref (GST_OBJECT (clock));
  gst_object_unref (GST_OBJECT (bin));

  exit (0);
}
