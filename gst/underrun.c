/* test audiosink underruns and how/if they are signaled.
 *
 * gcc -Wall underrun.c -o underrun `pkg-config gstreamer-1.0 --cflags --libs`
 *
 * GST_DEBUG="*:3,underrun:4,alsa:4" pasuspender ./underrun alsasink
 *
 * alsasink:
 *   nothing (there is a xrun_recovery() func, but I can't trigger it
 * jackaudiosink:
 *
 * pulsesink:
 *   pulsesink.c:702:gst_pulsering_stream_underflow_cb:<sink> Got underflow
 */

#include <stdio.h>
#include <stdlib.h>

#include <gst/gst.h>

#define SRC_NAME "audiotestsrc"
#define FX_NAME "volume"

#define GST_CAT_DEFAULT gst_test_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static void
event_loop (GstElement * bin)
{
  GstBus *bus = gst_element_get_bus (GST_ELEMENT (bin));
  GstMessage *message = NULL;

  while (TRUE) {
    message = gst_bus_poll (bus, GST_MESSAGE_ANY, -1);
    GST_INFO ("message received: %" GST_PTR_FORMAT, message);

    switch (message->type) {
      case GST_MESSAGE_EOS:
        gst_message_unref (message);
        return;
      case GST_MESSAGE_WARNING:
      case GST_MESSAGE_ERROR:{
        GError *gerror;
        gchar *debug;

        gst_message_parse_error (message, &gerror, &debug);
        gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
        gst_message_unref (message);
        g_error_free (gerror);
        g_free (debug);
        return;
      }
      default:
        gst_message_unref (message);
        break;
    }
  }
}

static GstPadProbeReturn
data_probe (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstBuffer *buf = GST_PAD_PROBE_INFO_BUFFER (info);
  GstClockTime ts = GST_BUFFER_TIMESTAMP (buf);
  GstClockTime dur = GST_BUFFER_DURATION (buf);

  GST_INFO_OBJECT (pad, "ts=%" GST_TIME_FORMAT ", dur=%" GST_TIME_FORMAT,
      GST_TIME_ARGS (ts), GST_TIME_ARGS (dur));

  // make sure buffers are late
  g_usleep (GST_TIME_AS_USECONDS (dur) * 4);
  //g_usleep (G_USEC_PER_SEC);
  return GST_PAD_PROBE_OK;
}

gint
main (gint argc, gchar ** argv)
{
  GstElement *pipeline, *src, *fx, *sink;
  GstPad *pad;
  GstStateChangeReturn res;
  gchar *sink_name = "pulsesink";

  /* init gstreamer */
  gst_init (&argc, &argv);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING);
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "underrun", 0, "underrun test");

  if (argc > 1) {
    sink_name = argv[1];
  }

  /* create a new bin to hold the elements */
  pipeline = gst_pipeline_new ("song");

  /* make elements and add them to the bin */
  if (!(src = gst_element_factory_make (SRC_NAME, "src"))) {
    fprintf (stderr, "Can't create element \"" SRC_NAME "\"\n");
    exit (-1);
  }
  if (!(fx = gst_element_factory_make (FX_NAME, "fx"))) {
    fprintf (stderr, "Can't create element \"" FX_NAME "\"\n");
    exit (-1);
  }
  if (!(sink = gst_element_factory_make (sink_name, "sink"))) {
    fprintf (stderr, "Can't create element \"%s\"\n", sink_name);
    exit (-1);
  }
  gst_bin_add_many (GST_BIN (pipeline), src, fx, sink, NULL);

  /* link elements */
  if (!gst_element_link_many (src, fx, sink, NULL)) {
    fprintf (stderr, "Can't link part1\n");
    exit (-1);
  }

  /* configure sink */
  /* FIXME: how do 'max-lateness' and 'discont-wait' relate? */
  g_object_set (sink, "qos", TRUE,
      "max-lateness", G_GUINT64_CONSTANT (0),
      "discont-wait", G_GUINT64_CONSTANT (0),
      "slave-method", 2, // none, default is 1=skew
      NULL);
  /* configure source */
  g_object_set (src, "num-buffers", 10, NULL);
  pad = gst_element_get_static_pad (src, "src");
  gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BUFFER, data_probe, NULL, NULL);
  gst_object_unref (pad);

  GST_INFO
      ("playing ================================================================");
  res = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (res == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "Can't go to playing\n");
    exit (-1);
  }

  event_loop (pipeline);

  /* stop the pipeline */
  GST_INFO
      ("exiting ================================================================");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (GST_OBJECT (pipeline));
  exit (0);
}