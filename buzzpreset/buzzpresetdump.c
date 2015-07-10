/* dump buzz preset files
 * gcc -Wall -g `pkg-config glib-2.0 --cflags --libs` buzzpresetdump.c -o buzzpresetdump
*/

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

gint
main(gint argc, gchar **argv)
{
  FILE *in;
  gchar *preset_path;
  
  if (argc<2) {
    puts ("Usage: buzzpresetdump <preset-file>");
    exit(1);
  }
  preset_path = argv[1];

  if ((in = fopen (preset_path, "rb"))) {
    guint32 i, version, size, count;
    guint32 tracks, params;
    guint32 *data;
    gchar *machine_name, *preset_name, *comment;

    // read header
    if (!(fread (&version, sizeof (version), 1, in)))
      goto eof_error;
    if (!(fread (&size, sizeof (size), 1, in)))
      goto eof_error;

    machine_name = g_malloc0 (size + 1);
    if (!(fread (machine_name, size, 1, in)))
      goto eof_error;

    if (!(fread (&count, sizeof (count), 1, in)))
      goto eof_error;

    printf ("reading %u presets for machine '%s' (version %u)\n",
        count, machine_name, version);

    // read presets
    for (i = 0; i < count; i++) {
      glong pos = ftell(in);
      if (!(fread (&size, sizeof (size), 1, in)))
        goto eof_error;

      preset_name = g_malloc0 (size + 1);
      if (!(fread (preset_name, size, 1, in)))
        goto eof_error;
      printf ("  reading preset %d @ %ld: %p '%s'\n", i, pos, preset_name, preset_name);
      if (!(fread (&tracks, sizeof (tracks), 1, in)))
        goto eof_error;
      if (!(fread (&params, sizeof (params), 1, in)))
        goto eof_error;

      printf ("    %u tracks, %u params\n", tracks, params);

      // read preset data
      printf ("    data size %u\n", (4 * (2 + params)));
      data = g_malloc (4 * (params));
      if (!(fread (data, 4 * params, 1, in)))
        goto eof_error;

      if (!(fread (&size, sizeof (size), 1, in)))
        goto eof_error;

      if (size) {
        comment = g_malloc0 (size + 1);
        if (!(fread (comment, size, 1, in)))
          goto eof_error;
          
        printf ("    comment '%s'\n", comment);
        g_free (comment);
      } else {
        comment = NULL;
      }

      g_free (data);
      g_free (preset_name);
    }
    g_free (machine_name);

eof_error:
    printf ("done or eof\n");
    fclose (in);
  } else {
    fprintf (stderr, "can't open preset file: '%s'\n", preset_path);
  }
  
  exit(0);
}
