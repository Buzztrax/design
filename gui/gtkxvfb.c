/* test gtk_init with custom display
 *
 * gcc -Wall -g gtkxvfb.c -o gtkxvfb `pkg-config gtk+-3.0 --cflags --libs`
 *
 * See https://bugzilla.gnome.org/show_bug.cgi?id=749752
 */

#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <sys/types.h>
#include <sys/wait.h>

static GPid server_pid;
static GdkDisplayManager *display_manager = NULL;
static GdkDisplay *default_display = NULL, *test_display = NULL;
static volatile gboolean wait_for_server;
static gchar display_name[3];
static gint display_number = -1;

static void
__test_server_watch (GPid pid, gint status, gpointer data)
{
  if (status == 0) {
    fprintf (stderr,"test x server %d process finished okay\n", pid);
  } else {
    fprintf (stderr, "test x server %d process finished with error %d\n", pid,
        status);
  }
  wait_for_server = FALSE;
  g_spawn_close_pid (pid);
  server_pid = 0;
}

static void
check_setup_test_server (void)
{
  //gulong flags=G_SPAWN_SEARCH_PATH|G_SPAWN_STDOUT_TO_DEV_NULL|G_SPAWN_STDERR_TO_DEV_NULL;
  gulong flags = G_SPAWN_SEARCH_PATH;
  GError *error = NULL;
  gchar display_file[18];
  gchar lock_file[14];
  // we can also use Xnest, but the without "-screen"
  gchar *server_argv[] = {
    "Xvfb",
    //"Xnest",
    ":9",
    "-ac",
    "-nolisten", "tcp",
#ifdef XFONT_PATH
    "-fp", XFONT_PATH,          /*"/usr/X11R6/lib/X11/fonts/misc" */
#endif
    "-noreset",
    /*"-terminate", */
    /* 32 bit does not work */
    "-screen", "0", "1024x786x24",
    "+extension", "RANDR",
    //"-render",                  /*"color", */
    NULL
  };
  gboolean found = FALSE, launched = FALSE, trying = TRUE;

  server_pid = 0;

  // allow running the test without Xvfb even though we have it.
  if (g_getenv ("BT_CHECK_NO_XVFB"))
    return;

  display_number = 0;
  // try display ids starting with '0'
  while (trying) {
    wait_for_server = TRUE;
    g_sprintf (display_name, ":%1d", display_number);
    g_sprintf (display_file, "/tmp/.X11-unix/X%1d", display_number);
    g_sprintf (lock_file, "/tmp/.X%1d-lock", display_number);

    // if we have a lock file, check if there is an alive process
    if (g_file_test (lock_file, G_FILE_TEST_EXISTS)) {
      FILE *pid_file;
      gchar pid_str[20];
      guint pid;
      gchar proc_file[15];

      // read pid, normal X11 is owned by root
      if ((pid_file = fopen (lock_file, "rt"))) {
        gchar *pid_str_res = fgets (pid_str, 20, pid_file);
        fclose (pid_file);

        if (pid_str_res) {
          pid = atol (pid_str);
          g_sprintf (proc_file, "/proc/%d", pid);
          // check proc entry
          if (!g_file_test (proc_file, G_FILE_TEST_EXISTS)) {
            // try to remove file and reuse display number
            if (!g_unlink (lock_file) && !g_unlink (display_file)) {
              found = TRUE;
            }
          }
        }
      }
    } else {
      found = TRUE;
    }

    // this display is not yet in use
    if (found && !g_file_test (display_file, G_FILE_TEST_EXISTS)) {
      // create the testing server
      server_argv[1] = display_name;
      if (!(g_spawn_async (NULL, server_argv, NULL, flags, NULL, NULL,
                  &server_pid, &error))) {
        fprintf (stderr, "error creating virtual x-server : \"%s\"\n", error->message);
        g_error_free (error);
      } else {
        g_child_watch_add (server_pid, __test_server_watch, NULL);

        while (wait_for_server) {
          // try also waiting for /tmp/X%1d-lock" files
          if (g_file_test (display_file, G_FILE_TEST_EXISTS)) {
            wait_for_server = trying = FALSE;
            launched = TRUE;
          } else {
            g_usleep (G_USEC_PER_SEC);
          }
        }
      }
    }
    if (!launched) {
      display_number++;
      // stop after trying the first ten displays
      if (display_number == 10)
        trying = FALSE;
    }
  }
  if (!launched) {
    display_number = -1;
    fprintf (stderr, "no free display number found\n");
  } else {
    // we still get a dozen of
    //   Xlib:  extension "RANDR" missing on display ...
    // this is a gdk bug:
    //   https://bugzilla.gnome.org/show_bug.cgi?id=645856
    //   and it seems to be fixed in Jan.2011

    //printf("####### Server started  \"%s\" is up (pid=%d)\n",display_name,server_pid);
    g_setenv ("DISPLAY", display_name, TRUE);
    fprintf (stderr,"test server \"%s\" is up (pid=%d)\n", display_name, server_pid);
    /* a window manager is not that useful
       gchar *wm_argv[]={"metacity", "--sm-disable", NULL };
       if(!(g_spawn_async(NULL,wm_argv,NULL,flags,NULL,NULL,NULL,&error))) {
       GST_WARNING("error running window manager : \"%s\"\n", error->message);
       g_error_free(error);
       }
     */
    /* this is not working
     ** (gnome-settings-daemon:17715): WARNING **: Failed to acquire org.gnome.SettingsDaemon
     ** (gnome-settings-daemon:17715): WARNING **: Could not acquire name
     gchar *gsd_argv[]={"/usr/lib/gnome-settings-daemon/gnome-settings-daemon", NULL };
     if(!(g_spawn_async(NULL,gsd_argv,NULL,flags,NULL,NULL,NULL,&error))) {
     GST_WARNING("error gnome-settings daemon : \"%s\"\n", error->message);
     g_error_free(error);
     }
     */
  }
}

static void
check_setup_test_display (void)
{
  if (display_number > -1) {
    // activate the display for use with gtk
    if ((display_manager = gdk_display_manager_get ())) {
      GdkScreen *default_screen;
      GtkSettings *default_settings;
      gchar *theme_name;

      default_display =
          gdk_display_manager_get_default_display (display_manager);
      if ((default_screen = gdk_display_get_default_screen (default_display))) {
        /* this block when protected by gdk_threads_enter() and crashes if not :( */
        //gdk_threads_enter();
        if ((default_settings = gtk_settings_get_for_screen (default_screen))) {
          g_object_get (default_settings, "gtk-theme-name", &theme_name, NULL);
          fprintf (stderr,"current theme is \"%s\"\n", theme_name);
          //g_object_unref(default_settings);
        } else
          fprintf (stderr, "can't get default_settings\n");
        //gdk_threads_leave();
        //g_object_unref(default_screen);
      } else
        fprintf (stderr, "can't get default_screen\n");

      if ((test_display = gdk_display_open (display_name))) {
#if 0
        GdkScreen *test_screen;

        if ((test_screen = gdk_display_get_default_screen (test_display))) {
          GtkSettings *test_settings;

          if ((test_settings = gtk_settings_get_for_screen (test_screen))) {
            // this just switches to the default theme
            //g_object_set(test_settings,"gtk-theme-name",NULL,NULL);
            /* Is there a bug in gtk+? None of this reliable creates a working
             * theme setup
             */
            //g_object_set(test_settings,"gtk-theme-name",theme_name,NULL);
            /* Again this shows no effect */
            //g_object_set(test_settings,"gtk-toolbar-style",GTK_TOOLBAR_ICONS,NULL);
            //gtk_rc_reparse_all_for_settings(test_settings,TRUE);
            //gtk_rc_reset_styles(test_settings);
            fprintf (stderr, "theme switched\n");
            //g_object_unref(test_settings);
          } else
            fprintf (stderr, "can't get test_settings on display: \"%s\"\n",
                display_name);
          //g_object_unref(test_screen);
        } else
          fprintf (stderr, "can't get test_screen on display: \"%s\"\n",
              display_name);
#endif
        gdk_display_manager_set_default_display (display_manager, test_display);
        fprintf (stderr,"display %p,\"%s\" is active\n", test_display,
            gdk_display_get_name (test_display));
      } else {
        fprintf (stderr, "failed to open display: \"%s\"\n", display_name);
      }
      //g_free(theme_name);
    } else {
      fprintf (stderr, "can't get display-manager\n");
    }
  }
}

static void
check_shutdown_test_display (void)
{
  if (test_display) {
    wait_for_server = TRUE;

    g_assert (GDK_IS_DISPLAY_MANAGER (display_manager));
    g_assert (GDK_IS_DISPLAY (test_display));
    g_assert (GDK_IS_DISPLAY (default_display));

    fprintf (stderr, "trying to shut down test display on server %d\n", server_pid);
    // restore default and close our display
    fprintf (stderr, "display_manager=%p, test_display=%p,\"%s\" "
        "default_display=%p,\"%s\"\n",
        display_manager, test_display, gdk_display_get_name (test_display),
        default_display, gdk_display_get_name (default_display));
    gdk_display_manager_set_default_display (display_manager, default_display);
    fprintf (stderr,"display has been restored\n");
    // TODO(ensonic): here it hangs, hmm not anymore
    //gdk_display_close(test_display);
    /* gdk_display_close() does basically the following (which still hangs):
       //g_object_run_dispose (G_OBJECT (test_display));
       fprintf (stderr,"test_display has been disposed\n");
       //g_object_unref (test_display);
     */
    fprintf (stderr,"display has been closed\n");
    test_display = NULL;
  } else {
    fprintf (stderr, "no test display\n");
  }
}

static void
check_shutdown_test_server (void)
{
  if (server_pid) {
    guint wait_count = 5;
    wait_for_server = TRUE;
    fprintf (stderr, "shutting down test server\n");

    // kill the testing server - TODO(ensonic): try other signals (SIGQUIT, SIGTERM).
    kill (server_pid, SIGINT);
    // wait for the server to finish (use waitpid() here ?)
    while (wait_for_server && wait_count) {
      g_usleep (G_USEC_PER_SEC);
      wait_count--;
    }
    server_pid = 0;
    fprintf (stderr, "test server has been shut down, wait_count=%d\n", wait_count);
  } else {
    fprintf (stderr, "no test server\n");
  }
}

static void
flush_main_loop (void)
{
  GMainContext *ctx = g_main_context_default ();

  fprintf (stderr, "flushing pending events ...\n");
  while (g_main_context_pending (ctx))
    g_main_context_iteration (ctx, FALSE);
  fprintf (stderr, "... done\n");
}

static void
save_screenshot (GtkWidget * widget)
{
  GdkWindow *window = gtk_widget_get_window (widget);
  GdkPixbuf *pixbuf;
  gint ww, wh;
  cairo_surface_t *surface;
  cairo_t *cr;

  // make sure the window gets drawn
  if (!gtk_widget_get_visible (widget)) {
    gtk_widget_show_all (widget);
  }
  if (GTK_IS_WINDOW (widget)) {
    gtk_window_present (GTK_WINDOW (widget));
  }
  gtk_widget_queue_draw (widget);
  flush_main_loop ();

  gdk_window_get_geometry (window, NULL, NULL, &ww, &wh);
  pixbuf = gdk_pixbuf_get_from_window (window, 0, 0, ww, wh);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, ww, wh);
  cr = cairo_create (surface);
  gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
  cairo_paint (cr);
  cairo_surface_write_to_png (surface, "gtkxvfb.png");
}

gint
main (gint argc, gchar * argv[])
{
  GtkWidget *window;

  check_setup_test_server ();
  
  gint pid = fork ();
  if (pid == 0) {
    fprintf (stderr,"child started\n");
    /*child*/
    gdk_init (&argc, &argv);
    check_setup_test_display ();
    gtk_init (&argc, &argv);
    fprintf (stderr,"child init done\n");

    if ((window = gtk_window_new (GTK_WINDOW_TOPLEVEL))) {
      gtk_container_add (GTK_CONTAINER (window), gtk_label_new ("hello"));
      gtk_widget_show_all (window);
      fprintf (stderr,"window created and showing\n");
      save_screenshot (window);
      g_usleep (G_USEC_PER_SEC);
      fprintf (stderr,"child stopping\n");
      gtk_widget_destroy (window);
      fprintf (stderr,"window closed\n");
    } else {
      fprintf (stderr,"child could not open window\n");
    }
    check_shutdown_test_display ();
  } else if (pid > 0) {
    waitpid (pid, NULL, 0);
    check_shutdown_test_server ();
  }
  return 0;
}
