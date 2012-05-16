/* test seemles looping in gstreamer
 *
 * In buzztard the loops are not smooth. The code below tries to reproduce the
 * issue. This emulates the whole stucture in buzztard, loop2.c has a cut-down
 * version that shows the problem.
 *
 * gcc -g `pkg-config gstreamer-0.10 gstreamer-controller-0.10 --cflags --libs` loop3.c -o loop3
 */

#include <stdio.h>
#include <gst/gst.h>
#include <gst/controller/gstcontroller.h>

/* configuration */
static gchar *src_names[]={
  "simsyn",
  "volume",
  "level",
  "tee",
  NULL
};
static gchar *wire_names[]={
  "queue",
  "tee",
  "volume",
  "audiopanorama",
  NULL
};
static gchar *sink_names[]={
  "adder",
  "audioconvert",
  "level",
  "volume",
  "level",
  "audioresample",
  "pulsesink",
  NULL
};

/* global stuff */

#define GST_CAT_DEFAULT gst_test_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static GMainLoop *main_loop=NULL;
static GstEvent *play_seek_event=NULL;
static GstEvent *loop_seek_event=NULL; 

static void message_received (GstBus * bus, GstMessage * message, GstPipeline * pipeline) {
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
  }
  else {
    puts ("no message details");
  }

  g_main_loop_quit(main_loop);
}

static void state_changed(const GstBus * const bus, GstMessage *message,  GstElement *bin) {
  if(GST_MESSAGE_SRC(message) == GST_OBJECT(bin)) {
    GstStateChangeReturn res;
    GstState oldstate,newstate,pending;

    gst_message_parse_state_changed(message,&oldstate,&newstate,&pending);
    switch(GST_STATE_TRANSITION(oldstate,newstate)) {
      case GST_STATE_CHANGE_READY_TO_PAUSED:
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(bin),GST_DEBUG_GRAPH_SHOW_ALL,"loop2");
        // seek to start time
        GST_INFO("initial seek ===========================================================");
        if(!(gst_element_send_event(bin,play_seek_event))) {
          fprintf(stderr,"element failed to handle seek event");
          g_main_loop_quit(main_loop);
        }
        // start playback
        GST_INFO("start playing ==========================================================");
        res=gst_element_set_state(bin,GST_STATE_PLAYING);
        if(res==GST_STATE_CHANGE_FAILURE) {
          fprintf(stderr,"can't go to playing state\n");
          g_main_loop_quit(main_loop);
        }
        else if(res==GST_STATE_CHANGE_ASYNC) {
          GST_INFO("->PLAYING needs async wait");
        }
        break;
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        GST_INFO("playback started =======================================================");
        break;
      default:
        break;
    }
  }
}

static void segment_done(const GstBus * const bus, const GstMessage * const message,  GstElement *bin) {
  GST_INFO("loop playback ==========================================================");
  if(!(gst_element_send_event(bin,gst_event_ref(loop_seek_event)))) {
    fprintf(stderr,"element failed to handle continuing play seek event\n");
    g_main_loop_quit(main_loop);
  }
}

static GstElement *make_src(void) {
  GstElement *b,*e[G_N_ELEMENTS(src_names)];
  GstPad *tp,*gp;
  guint i;
  GstController *ctrl;
  GValue val = { 0, };

  /* create and link elements */
  b = gst_element_factory_make ("bin", NULL);
  for(i=0;src_names[i];i++) {
    e[i]=gst_element_factory_make (src_names[i], NULL);
    gst_bin_add(GST_BIN(b),e[i]);
    if(i>0) {
      gst_element_link(e[i-1],e[i]);
    }
  }

  g_object_set(e[0], "wave", 2, NULL);
  
  /* setup controller */
  g_value_init (&val, G_TYPE_STRING);
  if (!(ctrl = gst_controller_new (G_OBJECT (e[0]), "note", NULL))) {
    fprintf(stderr,"can't control source element");exit (-1);
  }
  gst_controller_set_interpolation_mode (ctrl, "note", GST_INTERPOLATE_NONE);
  /* set control values */
  g_value_set_string (&val, "c-2");
  gst_controller_set (ctrl, "note",    0 * GST_MSECOND, &val);
  g_value_set_string (&val, "c-3");
  gst_controller_set (ctrl, "note",  250 * GST_MSECOND, &val);
  g_value_set_string (&val, "c-2");
  gst_controller_set (ctrl, "note",  500 * GST_MSECOND, &val);
  g_value_set_string (&val, "d-3");
  gst_controller_set (ctrl, "note",  750 * GST_MSECOND, &val);
  g_value_set_string (&val, "c-2");
  gst_controller_set (ctrl, "note", 1000 * GST_MSECOND, &val);
  g_value_set_string (&val, "d#3");
  gst_controller_set (ctrl, "note", 1250 * GST_MSECOND, &val);
  g_value_set_string (&val, "c-2");
  gst_controller_set (ctrl, "note", 1500 * GST_MSECOND, &val);
  g_value_set_string (&val, "g-3");
  gst_controller_set (ctrl, "note", 1750 * GST_MSECOND, &val);

  /* pads */
  if(!(tp=gst_element_get_request_pad(e[i-1],"src%d"))) {
    tp=gst_element_get_static_pad(e[i-1],"src");
  }
  gp=gst_ghost_pad_new("src",tp);
  gst_pad_set_active(gp,TRUE);
  gst_element_add_pad(b,gp);
  
  return b;  
}

static GstElement *make_wire(void) {
  GstElement *b,*e[G_N_ELEMENTS(wire_names)];
  GstPad *tp,*gp;
  guint i;

  /* create and link elements */
  b = gst_element_factory_make ("bin", NULL);
  for(i=0;wire_names[i];i++) {
    e[i]=gst_element_factory_make (wire_names[i], NULL);
    gst_bin_add(GST_BIN(b),e[i]);
    if(i>0) {
      gst_element_link(e[i-1],e[i]);
    }
  }

  /* pads */
  tp=gst_element_get_static_pad(e[i-1],"src");
  gp=gst_ghost_pad_new("src",tp);
  gst_pad_set_active(gp,TRUE);
  gst_element_add_pad(b,gp);
  
  tp=gst_element_get_static_pad(e[0],"sink");
  gp=gst_ghost_pad_new("sink",tp);
  gst_pad_set_active(gp,TRUE);
  gst_element_add_pad(b,gp);
  
  return b;  
}

static GstElement *make_sink(void) {
  GstElement *b,*e[G_N_ELEMENTS(sink_names)];
  GstPad *tp,*gp;
  guint i;
  gint64 chunk;

  /* create and link elements */
  b = gst_element_factory_make ("bin", NULL);
  for(i=0;sink_names[i];i++) {
    e[i]=gst_element_factory_make (sink_names[i], NULL);
    gst_bin_add(GST_BIN(b),e[i]);
    if(i>0) {
      gst_element_link(e[i-1],e[i]);
    }
  }

  /* configure latency */
  chunk=GST_TIME_AS_USECONDS((GST_SECOND*60)/(120*4));
  GST_INFO("changing audio chunk-size for sink to %"G_GUINT64_FORMAT" µs = %"G_GUINT64_FORMAT" ms",
    chunk, (chunk/G_GINT64_CONSTANT(1000)));
  g_object_set(e[i-1],
    "latency-time",chunk,
    "buffer-time",chunk<<1,
    NULL);

  /* pads */
  if(!(tp=gst_element_get_request_pad(e[0],"sink%d"))) {
    tp=gst_element_get_static_pad(e[0],"sink");
  }
  gp=gst_ghost_pad_new("sink",tp);
  gst_pad_set_active(gp,TRUE);
  gst_element_add_pad(b,gp);
  
  return b;  
}


int main(int argc, char **argv) {
  GstElement *bin;
  GstElement *src,*wire,*sink;
  GstBus *bus;
  GstStateChangeReturn res;
  
  /* init gstreamer */
  gst_init(&argc, &argv);
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "loop", 0, "loop test");

  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");
  /* see if we get errors */
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK(message_received), bin);
  g_signal_connect (bus, "message::warning", G_CALLBACK(message_received), bin);
  g_signal_connect (bus, "message::eos", G_CALLBACK(message_received), bin);
  g_signal_connect (bus, "message::segment-done", G_CALLBACK(segment_done), bin);
  g_signal_connect (bus, "message::state-changed", G_CALLBACK(state_changed), bin);
  gst_object_unref (G_OBJECT (bus));

  main_loop=g_main_loop_new(NULL,FALSE);

  /* make elements and add them to the bin */
  if(!(src = make_src ())) {
    fprintf(stderr,"Can't create source bin\n");exit (-1);
  }
  gst_bin_add (GST_BIN (bin), src);
  if(!(wire = make_wire ())) {
    fprintf(stderr,"Can't crate wire bin\n");exit (-1);
  }
  gst_bin_add (GST_BIN (bin), wire);
  if(!(sink = make_sink ())) {
    fprintf(stderr,"Can't crate sink bin\n");exit (-1);
  }
  gst_bin_add (GST_BIN (bin), sink);

  /* link elements */
  if(!gst_element_link_many (src, wire, sink, NULL)) {
    fprintf(stderr,"Can't link elements\n");exit (-1);
  }

  /* initial seek event */
  play_seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
        GST_SEEK_TYPE_SET, (GstClockTime)0,
        GST_SEEK_TYPE_SET, (GstClockTime)(2 * GST_SECOND));
  /* loop seek event (without flush) */
  loop_seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_SEGMENT,
        GST_SEEK_TYPE_SET, (GstClockTime)0,
        GST_SEEK_TYPE_SET, (GstClockTime)(2 * GST_SECOND));

  /* prepare playing */
  GST_INFO("prepare playing ========================================================");
  res=gst_element_set_state(bin, GST_STATE_PAUSED);
  if(res==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't go to paused\n");exit(-1);
  }
  else if(res==GST_STATE_CHANGE_ASYNC) {
    GST_INFO("->PAUSED needs async wait");
  }
  g_main_loop_run(main_loop);

  /* stop the pipeline */
  GST_INFO("exiting ================================================================");
  gst_element_set_state(bin, GST_STATE_NULL);

  /* we don't need a reference to these objects anymore */
  gst_object_unref(bin);
  g_main_loop_unref(main_loop);

  exit (0);
}
