/* progressbar packing
 *
 * gcc -Wall -g progbar.c -o progbar `pkg-config gtk+-3.0 --cflags --libs`
 *
 * The progressbar is supposed to be full height. This is the case in gtk+3.10.8
 * but not in 3.14.15.
 *
 * Changed in 74405cc. Only "solution" seems to center the widget.
 */

#include <gtk/gtk.h>
#include <glib.h>

gint
main (gint argc, gchar * argv[])
{
  GtkWidget *window, *box;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "horizontal progress bar layout");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit),
      NULL);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_box_pack_start (GTK_BOX (box), gtk_label_new ("Label"), TRUE, TRUE, 1);

  gtk_box_pack_start (GTK_BOX (box), gtk_progress_bar_new (), FALSE, FALSE, 1);

  gtk_box_pack_start (GTK_BOX (box), gtk_button_new_with_label ("Button"), TRUE,
      TRUE, 1);

  gtk_widget_show_all (window);
  gtk_main ();

  return 0;
}