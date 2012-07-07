/* Record n seconds of beep to a file
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` encode.c -o encode
 */

#include <gst/gst.h>

#include <stdio.h>
#include <stdlib.h>

static void
event_loop (GstElement * bin)
{
  GstBus *bus = gst_element_get_bus (GST_ELEMENT (bin));
  GstMessage *message = NULL;

  while (TRUE) {
    message = gst_bus_poll (bus, GST_MESSAGE_ANY, -1);   
    GST_INFO("message %s received",GST_MESSAGE_TYPE_NAME(message));

    switch (message->type) {
      case GST_MESSAGE_EOS:
        gst_message_unref (message);
        return;
      case GST_MESSAGE_WARNING:
      case GST_MESSAGE_ERROR: {
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


gint
main (gint argc, gchar ** argv)
{
  GstElement *bin;
  gchar *pipeline = NULL;
  gint format = 0;
  
  gst_init (&argc, &argv);
  
  if(argc>1) {
    format=atoi(argv[1]);
  }
  switch(format) {
    case 0:
      pipeline = "audiotestsrc ! vorbisenc ! oggmux ! filesink location=encode.ogg";
      break;
    case 1:
      pipeline = "audiotestsrc ! lamemp3enc ! filesink location=encode.mp3";
      break;
    case 2:
      pipeline = "audiotestsrc ! wavenc ! filesink location=encode.wav";
      break;
    case 3:
      pipeline = "audiotestsrc ! flacenc ! oggmux ! filesink location=encode.flac";
      break;
    case 4:
      pipeline = "audiotestsrc ! faac ! mp4mux ! filesink location=encode.m4a";
      break;
    case 5:
      pipeline = "audiotestsrc ! filesink location=encode.raw";
      break;
    default:
      puts("format must be 0-5");
      return -1;
  }
  bin = gst_parse_launch (pipeline, NULL);
  
  gst_element_set_state (GST_ELEMENT (bin),GST_STATE_PAUSED);
  if(!gst_element_seek (bin, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
      GST_SEEK_TYPE_SET, GST_SECOND*0,
      GST_SEEK_TYPE_SET, GST_SECOND*1))
    puts("seek failed");
  
  gst_element_set_state (GST_ELEMENT (bin),GST_STATE_PLAYING);
  event_loop (bin);
  gst_element_set_state (bin, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (bin));
  return 0;
}
