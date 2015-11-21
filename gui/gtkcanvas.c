/*
 * gtk canvas test using a gtklayout
 *
 * gcc -Wall -g gtkcanvas.c -o gtkcanvas `pkg-config gtk+-3.0 --cflags  --libs`
 *
 * TODO:
 * - add background grid
 * - make widgets movable
 * - add wires (drawable?)
 * - figure out how to manage z-order
 * - add image effects (transparency, shading)
 */

#include <gtk/gtk.h>

#define WIDTH 320
#define HEIGHT 240

gint
main (gint argc, gchar * argv[])
{
  GtkWidget *window = NULL;
  GtkWidget *scrolled_window = NULL;
  GtkWidget *canvas;
  GtkWidget *child1, *child2;
  GtkIconTheme *it = NULL;

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

  canvas = gtk_layout_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window), canvas);
  gtk_layout_set_size (GTK_LAYOUT (canvas), WIDTH, HEIGHT);
  gtk_scrolled_window_set_min_content_width (GTK_SCROLLED_WINDOW (scrolled_window), WIDTH/2);
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scrolled_window), HEIGHT/2);

  child1 = gtk_button_new_with_label ("Hello");
  gtk_layout_put (GTK_LAYOUT (canvas), child1, WIDTH/2, HEIGHT/2);

  child2 = gtk_image_new_from_pixbuf (gtk_icon_theme_load_icon (it,
      "buzztrax_generator", 64,
      GTK_ICON_LOOKUP_FORCE_SVG | GTK_ICON_LOOKUP_FORCE_SIZE, NULL));
  gtk_layout_put (GTK_LAYOUT (canvas), child2, WIDTH/4, HEIGHT/4);

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
