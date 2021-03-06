/* small example on how to use clocks
 */

#include <stdio.h>
#include <gst/gst.h>
#include <gst/control/control.h>

gfloat set_to_value;
GValue *set_val;
GstDParam *volume;

/* clock function. This function is called on each clock cycle */
gboolean
clockFunc (GstClock * clock, GstClockTime time, GstClockID id,
    gpointer user_data)
{
  g_print ("tick : volume=%f\n", set_to_value);
  set_to_value -= 0.1;
  g_value_set_float (set_val, set_to_value);
  g_object_set_property (G_OBJECT (volume), "value_float", set_val);
  if (set_to_value < 0.1) {
    g_print ("clock end\n");
    gst_main_quit ();
  }
  g_print ("clock::is_active: %d\n", gst_clock_is_active (clock));
  g_print ("entry::status: %d (ok=0,early,restart)\n",
      GST_CLOCK_ENTRY_STATUS ((GstClockEntry *) id));

  return TRUE;
}

int
main (int argc, char **argv)
{
  GstElement *audiosink, *audiosrc, *audioconvert, *thread;
  GstClock *clock;
  GstClockID *clockId;
  GstDParamManager *audiosrcParam;

  gst_init (&argc, &argv);
  gst_control_init (&argc, &argv);

  if (argc < 3) {
    g_print
        ("usage: %s <src> <sink> <value>\nvalue should be between 0 ... 100\n",
        argv[0]);
    g_print ("example: %s audiotestsrc esdsink 100\n", argv[0]);
    exit (-1);
  }

  /* create a new thread to hold the elements */
  thread = gst_thread_new ("thread");
  g_assert (thread != NULL);

  /* and an audio sink */
  audiosink = gst_element_factory_make (argv[2], "play_audio");
  g_assert (audiosrc != NULL);

  audioconvert = gst_element_factory_make ("audioconvert", "convert");

  /* add an sine source */
  audiosrc = gst_element_factory_make (argv[1], "audio-source");
  g_assert (audiosrc != NULL);

  set_to_value = atof (argv[3]);
  set_to_value = set_to_value / 100;
  /* getting param manager */
  audiosrcParam = gst_dpman_get_manager (audiosrc);
  /* setting param mode. Only synchronized currently supported */
  gst_dpman_set_mode (audiosrcParam, "synchronous");
  volume = gst_dparam_new (G_TYPE_FLOAT);
  if (gst_dpman_attach_dparam (audiosrcParam, "volume", volume)) {
    /* the dparam was successfully attached */
    set_val = g_new0 (GValue, 1);
    g_value_init (set_val, G_TYPE_FLOAT);
    g_value_set_float (set_val, set_to_value);
    g_object_set_property (G_OBJECT (volume), "value_float", set_val);
  }

  /* add objects to the thread */
  gst_bin_add_many (GST_BIN (thread), audiosrc, audioconvert, audiosink, NULL);
  /* link them in the logical order */
  gst_element_link_many (audiosrc, audioconvert, audiosink, NULL);

  /* getting the clock */
  //gst_bin_auto_clock(GST_BIN(thread));
  g_print ("audiosink provides clock? %d\n",
      gst_element_provides_clock (audiosink));
  if (gst_element_provides_clock (audiosink)) {
    gst_bin_use_clock (GST_BIN (thread), gst_element_get_clock (audiosink));
    clock = gst_element_get_clock (audiosink);
    g_print ("using %s clock which is instance_of(%s)\n", argv[2],
        GST_OBJECT_NAME (clock));
  } else {
    gst_bin_use_clock (GST_BIN (thread), gst_system_clock_obtain ());
    clock = gst_bin_get_clock (GST_BIN (thread));
    g_print ("using system clock which is instance_of(%s)\n",
        GST_OBJECT_NAME (clock));
  }

  g_print ("clock::speed: %f\n", gst_clock_get_speed (clock));
  g_print ("clock::resolution: %llu\n", gst_clock_get_resolution (clock));
  g_print ("clock::is active: %d\n", gst_clock_is_active (clock));
  g_print
      ("clock::flags: %d (single_sync=1,single_asych=2,peri_sync=4,peri_asych=8,resolution=16,speed=32)\n",
      GST_CLOCK_FLAGS (clock));
  clockId = gst_clock_new_periodic_id (clock, GST_SECOND * 2, GST_SECOND * 1);
  g_print ("entry result: %d (error=0,timeout,early,error,unsupported)\n",
      gst_clock_id_wait_async (clockId, &clockFunc, NULL, NULL));
  g_print ("entry::status: %d (ok=0,early,restart)\n",
      GST_CLOCK_ENTRY_STATUS ((GstClockEntry *) clockId));

  /* start playing */
  gst_element_set_state (thread, GST_STATE_PLAYING);

  /* do whatever you want here, the thread will be playing */
  g_print ("thread is playing\n");

  gst_main ();

  /* stop the pipeline */
  gst_element_set_state (thread, GST_STATE_NULL);

  /* we don't need a reference to these objects anymore */
  gst_object_unref (GST_OBJECT (thread));

  exit (0);
}
