/*
 * test toolbar buttons
 *
 * gcc -Wall -g toolbutton.c -o toolbutton `pkg-config gtk+-3.0 --cflags  --libs`
 */

#include <gtk/gtk.h>

gint
main (gint argc, gchar * argv[])
{
  GtkWidget *window, *box;
  GtkToolbar *tb[GTK_TOOLBAR_BOTH_HORIZ+1];
  GtkToolItem *ti;
  GtkIconSize icon_size;
  gint i;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "toolbar buttons vs toolbar styles");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit),
      NULL);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (window), box);

  // foreach style
  for (i = GTK_TOOLBAR_ICONS; i <= GTK_TOOLBAR_BOTH_HORIZ; i++) {
    tb[i] = (GtkToolbar *)gtk_toolbar_new ();
    gtk_toolbar_set_style (tb[i], i);
    gtk_box_pack_start (GTK_BOX (box), (GtkWidget *)tb[i], TRUE, TRUE, 0);
    icon_size = gtk_tool_shell_get_icon_size (GTK_TOOL_SHELL (tb[i]));
    
    // create buttons
    ti = gtk_tool_button_new (gtk_image_new_from_icon_name ("help-about",
          icon_size), "About");
    gtk_toolbar_insert (tb[i], ti, -1);
    
    ti = g_object_new (GTK_TYPE_TOOL_BUTTON,
			 "icon-name", "help-about",
			 "label", "About",
			 NULL);
		gtk_toolbar_insert (tb[i], ti, -1);
		
		gtk_toolbar_insert (tb[i], gtk_separator_tool_item_new(), -1);

    ti = g_object_new (GTK_TYPE_TOGGLE_TOOL_BUTTON,
			 "icon-name", "help-about",
			 "label", "About",
			 "active", TRUE,
			 NULL);
		gtk_toolbar_insert (tb[i], ti, -1);
		
		gtk_box_pack_start (GTK_BOX (box), 
		    gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), TRUE, TRUE, 0);
  }

  gtk_widget_show_all (window);
  gtk_main ();

  return 0;
}
