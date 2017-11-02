/* test gst buffer clipping
 *
 * gcc -Wall clip.c -o clip `pkg-config gstreamer-1.0 --cflags --libs`
 */


#include <stdio.h>
#include <stdlib.h>

#include <gst/gst.h>

#define SRC_NAME "audiotestsrc"
//#define SRC_NAME "simsyn"
#define FX_NAME "volume"
#define SINK_NAME "autoaudiosink"

#define GST_CAT_DEFAULT gst_test_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

gint
main (gint argc, gchar ** argv)
{
  GstElement *pipeline, *src, *fx, *sink;
  GstStateChangeReturn res;
  GstEvent *play_seek_event;
  GstBus *bus;
  GstMessage *msg;
  gboolean loop = TRUE;
  gint samplesperbuffer = -1;

  /* init gstreamer */
  gst_init (&argc, &argv);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING);
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "clip", 0, "clip test");

  if (argc > 1) {
    samplesperbuffer = atoi (argv[1]);
  }

  /* create a new bin to hold the elements */
  pipeline = gst_pipeline_new ("song");

  /* make elements and add them to the bin */
  if (!(src = gst_element_factory_make (SRC_NAME, "src"))) {
    fprintf (stderr, "Can't create element \"" SRC_NAME "\"\n");
    exit (-1);
  }
  if (samplesperbuffer != -1) {
    g_object_set (src, "samplesperbuffer", samplesperbuffer, NULL);
  }
  if (!(fx = gst_element_factory_make (FX_NAME, "fx"))) {
    fprintf (stderr, "Can't create element \"" FX_NAME "\"\n");
    exit (-1);
  }
  if (!(sink = gst_element_factory_make (SINK_NAME, "sink"))) {
    fprintf (stderr, "Can't create element \"" SINK_NAME "\"\n");
    exit (-1);
  }
  gst_bin_add_many (GST_BIN (pipeline), src, fx, sink, NULL);

  /* link elements */
  if (!gst_element_link_many (src, fx, sink, NULL)) {
    fprintf (stderr, "Can't link part1\n");
    exit (-1);
  }

  /* play for n seconds */
  play_seek_event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH,
      GST_SEEK_TYPE_SET, (GstClockTime) 0,
      GST_SEEK_TYPE_SET, (GstClockTime) (4 * GST_SECOND));

  /* prepare playing */
  GST_INFO
      ("prepare playing ========================================================");
  res = gst_element_set_state (pipeline, GST_STATE_PAUSED);
  if (res == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "Can't go to paused\n");
    exit (-1);
  }

  bus = gst_element_get_bus (pipeline);
  while (loop) {
    msg = gst_bus_poll (bus,
        GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR |
        GST_MESSAGE_EOS, GST_CLOCK_TIME_NONE);
    if (!msg)
      continue;

    GST_DEBUG_OBJECT (GST_MESSAGE_SRC (msg), "bus msg: %" GST_PTR_FORMAT, msg);

    switch (msg->type) {
      case GST_MESSAGE_STATE_CHANGED:
        if (GST_MESSAGE_SRC (msg) == GST_OBJECT (pipeline)) {
          GstState oldstate, newstate;

          gst_message_parse_state_changed (msg, &oldstate, &newstate,
              NULL);
          if (GST_STATE_TRANSITION (oldstate,
                  newstate) == GST_STATE_CHANGE_READY_TO_PAUSED) {
            if (!(gst_element_send_event (pipeline, play_seek_event))) {
              fprintf (stderr, "failed to send seek event\n");
              exit (1);
            }
            GST_INFO
                ("playing ================================================================");
            res = gst_element_set_state (pipeline, GST_STATE_PLAYING);
            if (res == GST_STATE_CHANGE_FAILURE) {
              fprintf (stderr, "Can't go to paused\n");
              exit (-1);
            }
          }
        }
        break;
      case GST_MESSAGE_EOS:
      case GST_MESSAGE_ERROR:
        loop = FALSE;
        break;
      default:
        break;
    }
    gst_message_unref (msg);
  }

  /* stop the pipeline */
  GST_INFO
      ("exiting ================================================================");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (bus);
  gst_object_unref (GST_OBJECT (pipeline));
  exit (0);
}
