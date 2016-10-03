/* test gst context use for tempo distribution
 *
 * gcc -Wall context.c -o context `pkg-config gstreamer-1.0 --cflags --libs`
 */

#include <stdio.h>
#include <stdlib.h>

#include <gst/gst.h>

/* elements need to implement GstElement:set_context()
 */

gint
main (gint argc, gchar ** argv)
{
  GstElement *bin;
  GstContext *ctx;

  /* init gstreamer */
  gst_init (&argc, &argv);

#if 0
  // https://en.wikipedia.org/wiki/Tempo#Beats_per_minute
  // 120 bpm @ 4 tpb : 120 quarter (1/4) notes per minute
  // subticks are used to lower latencies (and have smother envelopes, lfos)

  #define GST_AUDIO_TEMPO_TYPE "gst.audio.Tempo"
  ctx = gst_context_new (GST_AUDIO_TEMPO_TYPE, FALSE);
  GstStructure *s = gst_context_writable_structure (ctx);
  gst_structure_set (s,
      "beats-per-minute", G_TYPE_UINT, bpm,
      "ticks-per-beat", G_TYPE_UINT, tpb,
      "subticks-per-beat", G_TYPE_UINT, stpb,
      NULL);

  gst_element_set_context (bin, ctx);
#endif

  exit (0);
}
