/*
 * gtk canvas test using a gtklayout
 *
 * gcc -Wall -g gtkcanvas.c -o gtkcanvas `pkg-config gtk+-3.0 --cflags  --libs`
 *
 * DONE:
 * - draw background (with scrolling support)
 * - add widgets
 * - make widgets movable
 *
 * HACK:
 * - figure out how to manage z-order
 *   - remove and add move a widget to top
 *
 * TODO:
 * - add connecting wires (drawable?)
 * - add image effects (transparency, shading)
 * - zoom
 * - gravity center for GtkLayout
 */

#include <gtk/gtk.h>

#define WIDTH 320
#define HEIGHT 240

static GtkIconTheme *it = NULL;
static GtkWidget *canvas = NULL;
static gboolean drag = FALSE;
static gdouble mxs = 0.0, mys = 0.0;
static gint cxs = 0, cys = 0;

static gboolean
on_canvas_draw (GtkWidget * widget, cairo_t * cr, gpointer user_data)
{
  GtkStyleContext *style_ctx;
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
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_rectangle (cr, -xpos, -ypos, width, height);
  cairo_stroke (cr);

  cairo_move_to (cr, -xpos, -ypos);
  cairo_line_to (cr, width-xpos, height-ypos);
  cairo_stroke (cr);

  cairo_move_to (cr, width-xpos, -ypos);
  cairo_line_to (cr, -xpos, height-ypos);
  cairo_stroke (cr);

  return FALSE; // continue to draw
}

static gboolean
on_machine_button_press_event (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  if (event->button == GDK_BUTTON_PRIMARY && event->type == GDK_BUTTON_PRESS) {
    drag = TRUE;
    mxs = event->x_root;
    mys = event->y_root;
    gtk_container_child_get (GTK_CONTAINER (canvas), widget, "x", &cxs, "y", &cys, NULL);
    // move child to top, hack?
    gtk_container_remove (GTK_CONTAINER (canvas), widget);
    gtk_layout_put (GTK_LAYOUT (canvas), widget, cxs, cys);
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
    drag = FALSE;
  }
  return FALSE;
}

GtkWidget *
make_machine (const char *name)
{
  GtkWidget *image = gtk_image_new_from_pixbuf (gtk_icon_theme_load_icon (it,
      name, 64,
      GTK_ICON_LOOKUP_FORCE_SVG | GTK_ICON_LOOKUP_FORCE_SIZE, NULL));

  GtkWidget *event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (event_box), image);

  gtk_widget_add_events (event_box, GDK_BUTTON1_MOTION_MASK);

  g_signal_connect (event_box, "button-press-event",
      G_CALLBACK (on_machine_button_press_event), NULL);
  g_signal_connect (event_box, "button-release-event",
      G_CALLBACK (on_machine_button_release_event), NULL);
  g_signal_connect (event_box, "motion-notify-event",
      G_CALLBACK (on_machine_motion_notify_event), NULL);
  return event_box;
}

gint
main (gint argc, gchar * argv[])
{
  GtkWidget *window = NULL;
  GtkWidget *scrolled_window = NULL;
  GtkWidget *child1, *child2;

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

  child1 = make_machine (/*"buzztrax_master"*/ "zoom-in");
  gtk_layout_put (GTK_LAYOUT (canvas), child1, WIDTH/2, HEIGHT/2);

  child2 = make_machine (/*"buzztrax_generator"*/ "zoom-out");
  gtk_layout_put (GTK_LAYOUT (canvas), child2, WIDTH/4, HEIGHT/4);

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
