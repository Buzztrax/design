/*
 * gtk canvas test using a gtklayout
 *
 * gcc -Wall -g gtkcanvas.c -o gtkcanvas `pkg-config gtk+-3.0 --cflags  --libs`
 *
 * DONE:
 * - draw background (with scrolling support)
 * - add widgets
 * - make widgets movable
 * - manage z-order (by drawing children ourself)
 *
 * TODO:
 * - manage z-order
 *   - we probbaly have to find a way to reorder the children list, since this
 *     is also used for events => we should just copy the GtkLayout code
 * - add connecting wires (drawable?)
 *   - rotation (via css? or gdk_pixbuf_rotate_simple())
 * - add image effects (transparency, shading)
 *   - gtk_image_get_pixbuf
 * - zoom
 * - gravity center for GtkLayout
 *
 * GTK-3.12:
 * - use gdk_window_set_event_compression(window)
 */

#include <gtk/gtk.h>

#define WIDTH 320
#define HEIGHT 240

static GtkIconTheme *it = NULL;
static GtkWidget *canvas = NULL;
static GtkWidget *drag = NULL;
static GtkWidget *icon_s, *icon_e;
static gdouble mxs = 0.0, mys = 0.0;
static gint cxs = 0, cys = 0;

static gboolean
on_canvas_draw (GtkWidget * widget, cairo_t * cr, gpointer user_data)
{
  GtkStyleContext *style_ctx;
  GtkWidget *child;
  GList *list, *node, *listm = NULL;
  guint width, height;
  guint xpos, ypos;

  gtk_layout_get_size (GTK_LAYOUT (widget), &width, &height);
  xpos = gtk_adjustment_get_value (gtk_scrollable_get_hadjustment (GTK_SCROLLABLE (widget)));
  ypos = gtk_adjustment_get_value (gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (widget)));
  style_ctx = gtk_widget_get_style_context (widget);

  /* draw border */
  gtk_render_background (style_ctx, cr, 0, 0, width, height);
  gtk_render_frame (style_ctx, cr, 0, 0, width, height);

  /* draw a box + cross, offset by xpos/ypos to support scrolling */
  cairo_set_source_rgb (cr, 0.5, 0.5, 0.5);
  cairo_rectangle (cr, -xpos, -ypos, width, height);
  cairo_stroke (cr);

  cairo_move_to (cr, -xpos, -ypos);
  cairo_line_to (cr, width-xpos, height-ypos);
  cairo_stroke (cr);

  cairo_move_to (cr, width-xpos, -ypos);
  cairo_line_to (cr, -xpos, height-ypos);
  cairo_stroke (cr);

  /* z-order redraw: wires, machines, moving machine */
  list = gtk_container_get_children ((GtkContainer *)widget);
  for (node = list; node; node = g_list_next (node)) {
    child = node->data;
    if (GTK_IS_EVENT_BOX (child)) {
      listm = g_list_prepend (listm, child);
    } else {
      gtk_container_propagate_draw ((GtkContainer *)widget, child, cr);
    }
  }
  g_list_free (list);
  for (node = listm; node; node = g_list_next (node)) {
    child = node->data;
    if (G_UNLIKELY (child == drag)) continue;
    gtk_container_propagate_draw ((GtkContainer *)widget, child, cr);
  }
  g_list_free (listm);
  if (drag) {
    gtk_container_propagate_draw ((GtkContainer *)widget, drag, cr);
  }
  return TRUE; // we're done
}

static gboolean
on_wire_draw (GtkWidget * widget, cairo_t * cr, gpointer user_data)
{
  guint width, height;

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  // FIXME: this should be a line
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_rectangle (cr, 0.0, 0.0, width, height);
  cairo_stroke (cr);

  return TRUE;
}


static gboolean
on_machine_button_press_event (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  if (event->button == GDK_BUTTON_PRIMARY && event->type == GDK_BUTTON_PRESS) {
    drag = widget;
    mxs = event->x_root;
    mys = event->y_root;
    gtk_container_child_get (GTK_CONTAINER (canvas), widget, "x", &cxs, "y", &cys, NULL);
  }
  return FALSE;
}

static gboolean
on_machine_motion_notify_event (GtkWidget * widget,
    GdkEventMotion * event, gpointer user_data)
{
  if (!drag)
    return TRUE;

  gdouble mxd = event->x_root - mxs, myd = event->y_root - mys;
  gtk_layout_move (GTK_LAYOUT (canvas), widget, cxs + mxd, cys + myd);
  return FALSE;
}

static gboolean
on_machine_button_release_event (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  if (event->button == GDK_BUTTON_PRIMARY && event->type == GDK_BUTTON_RELEASE) {
    drag = NULL;
  }
  return FALSE;
}

static GtkWidget *
add_machine (GtkLayout *layout, const char *name, gint x, gint y)
{
  GdkPixbuf *pix = gtk_icon_theme_load_icon (it, name, 64,
      GTK_ICON_LOOKUP_FORCE_SVG | GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
  GtkWidget *image = gtk_image_new_from_pixbuf (pix);
  g_object_unref (pix);

  GtkWidget *event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (event_box), image);

  gtk_widget_add_events (event_box, GDK_BUTTON1_MOTION_MASK);

  g_signal_connect (event_box, "button-press-event",
      G_CALLBACK (on_machine_button_press_event), NULL);
  g_signal_connect (event_box, "button-release-event",
      G_CALLBACK (on_machine_button_release_event), NULL);
  g_signal_connect (event_box, "motion-notify-event",
      G_CALLBACK (on_machine_motion_notify_event), NULL);

  gtk_layout_put (layout, event_box, x, y);

  return event_box;
}

static void
get_machine_pos (GtkWidget *child, gint *x, gint *y)
{
  gtk_container_child_get ((GtkContainer *)canvas, child, "x", x, "y", y, NULL);
  gint w = gtk_widget_get_allocated_width (child);
  gint h = gtk_widget_get_allocated_height (child);
  *x += w/2;
  *y += h/2;
}

static void
on_wire_changed (GtkWidget *child, GParamSpec * arg, gpointer user_data)
{
  GtkWidget *wire = (GtkWidget *)user_data;
  gint x1, x2, y1, y2;
  gint x, y, w, h;

  get_machine_pos (icon_s, &x1, &y1);
  get_machine_pos (icon_e, &x2, &y2);

  if (x2 > x1) {
    x = x1;
    w = x2 - x1;
  } else if (x2 < x1) {
    x = x2;
    w = x1 - x2;
  } else {
    x = x1;
    w = 1;
  }
  if (y2 > y1) {
    y = y1;
    h = y2 - y1;
  } else if (y2 < y1) {
    y = y2;
    h = y1 - y2;
  } else {
    y = y1;
    h = 1;
  }
  gtk_widget_set_size_request (wire, w, h);
  gtk_container_child_set ((GtkContainer *)canvas, wire, "x", x, "y", y, NULL);
}

static GtkWidget *
add_wire (GtkLayout *layout, GtkWidget *m1, GtkWidget *m2, const char *name)
{
  GtkWidget *wire = gtk_drawing_area_new ();

  // FIXME: we're supposed to call this in _init()
  // this is a hack for now to make the wire not take events
  gtk_widget_set_has_window (wire, FALSE);

  g_signal_connect (wire, "draw", G_CALLBACK (on_wire_draw), NULL);

  g_signal_connect (m1, "child-notify::x", G_CALLBACK (on_wire_changed), (gpointer) wire);
  g_signal_connect (m1, "child-notify::y", G_CALLBACK (on_wire_changed), (gpointer) wire);
  g_signal_connect (m2, "child-notify::x", G_CALLBACK (on_wire_changed), (gpointer) wire);
  g_signal_connect (m2, "child-notify::y", G_CALLBACK (on_wire_changed), (gpointer) wire);

  gtk_layout_put (layout, wire, 0, 0);
  on_wire_changed (NULL, NULL, wire);
  // FIXME: I mave to physically move it to make it properly redraw

  return wire;
}

gint
main (gint argc, gchar * argv[])
{
  GtkWidget *window = NULL;
  GtkWidget *scrolled_window = NULL;

  gtk_init (&argc, &argv);

  it = gtk_icon_theme_get_default ();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Canvas");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit),
      NULL);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (window), scrolled_window);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
  // make child centered if scrolled window is larger
  // gtk_alignment_set (GTK_ALIGNMENT (xxx), 0.5, 0.5, 0.0, 0.0);

  canvas = gtk_layout_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window), canvas);
  gtk_layout_set_size (GTK_LAYOUT (canvas), WIDTH, HEIGHT);
  gtk_scrolled_window_set_min_content_width (GTK_SCROLLED_WINDOW (scrolled_window), WIDTH/2);
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scrolled_window), HEIGHT/2);
  g_signal_connect (canvas, "draw", G_CALLBACK (on_canvas_draw), NULL);

  // machines
  icon_s = add_machine (GTK_LAYOUT (canvas), /*"buzztrax_master"*/ "zoom-in", WIDTH/4, HEIGHT/4);
  icon_e = add_machine (GTK_LAYOUT (canvas), /*"buzztrax_generator"*/ "zoom-out", WIDTH/2, HEIGHT/2);

  // wires
  add_wire (GTK_LAYOUT (canvas), icon_s, icon_e, "media-play");

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
