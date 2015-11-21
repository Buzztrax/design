/*
 * test the volumebutton
 * 
 * gcc -Wall -g volumebutton.c -o volumebutton `pkg-config gtk+-3.0 --cflags --libs`
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>

static void
destroy (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

gint
main (gint argc, gchar ** argv)
{
  GtkWidget *window, *vb;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Volumebutton");
  gtk_widget_set_size_request (GTK_WIDGET (window), 100, 100);
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);

  vb = gtk_volume_button_new ();

  gtk_container_add (GTK_CONTAINER (window), vb);

  gtk_widget_show_all (window);
  gtk_main ();

  return 0;
}
