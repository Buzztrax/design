/*
 * test multiscreens
 *
 * gcc -Wall -g multiscreen.c -o multiscreen `pkg-config gtk+-3.0 --cflags  --libs`
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

GdkRectangle r[10];

static void
add_window (GdkScreen *screen, gint num) {
  GtkWidget *window;
  gchar title[20], *name;
  gint xpos, ypos;
  
  name = gdk_screen_get_monitor_plug_name (screen, num);
  printf ("monitor name[%d]   : %s\n",  num, name);
  g_free (name);
  gdk_screen_get_monitor_geometry (screen, num, &r[num]);
  printf ("monitor pos[%d]    : %d,%d\n",  num, r[num].x, r[num].y);
  printf ("monitor size[%d]   : %d,%d\n",  num, r[num].width, r[num].height);

  sprintf (title, "monitor %d", num);
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), title);
  gtk_window_get_position (GTK_WINDOW (window), &xpos, &ypos);
  xpos += r[num].x;
  ypos = MAX (ypos, r[num].y);
  printf ("window pos[%d]     : %d,%d\n\n",  num, xpos, ypos);
  gtk_window_move (GTK_WINDOW (window), xpos, ypos);
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit),
      NULL);

  gtk_widget_show_all (window);
}

gint
main (gint argc, gchar * argv[])
{
  GdkDisplay *display;
  GdkScreen *screen;
  gint i, num;

  gtk_init (&argc, &argv);
  
  display = gdk_display_get_default ();
  screen = gdk_display_get_default_screen (display);
  
  num = gdk_screen_get_n_monitors (screen);
  num = MIN (num, 10);
  
  printf ("display name     : '%s'\n",  gdk_display_get_name (display));
  printf ("number of monitors: %d\n\n",  num);

  for (i = 0; i < num; i++) {
    add_window (screen, i);
  }
  gtk_main ();

  return 0;
}