/* repair buzz preset files
 * gcc -Wall -g `pkg-config glib-2.0 --cflags --libs` buzzpresetrepair.c -o buzzpresetrepair
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

static glong
scan_data(gchar *path, gchar *data, gint len)
{
  FILE *in;
  glong res = -1;
  
  if ((in = fopen (path, "rb"))) {
    gchar *buf = g_malloc(len);
    glong pos = 0;
    gint len1 = len - 1;

    fread (buf, len, 1, in);
    while (!feof (in)) {
      if (fread (&buf[len1], 1, 1, in) == 1) {
        if (!memcmp(buf, data, len)) {
          res = pos;
          break;
        }
        memmove(buf, &buf[1], len1);
        pos++;
      }
    }
    g_free(buf);
    fclose (in);
  }
  return res;
}

gint
main(gint argc, gchar **argv)
{
  FILE *in;
  gchar *preset_path, *garbage_path;
  
  if (argc<3) {
    puts ("Usage: buzzpresetrepair <fine-preset-file> <broken-preset-file>");
    exit(1);
  }
  preset_path = argv[1];
  garbage_path = argv[2];

  if ((in = fopen (preset_path, "rb"))) {
    guint32 i, j, version, size, count, blob_size, name_size, comment_size;
    guint32 tracks, params;
    guint32 *data;
    gchar *machine_name, *preset_name, *comment;
    gchar *blob;

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
      } else {
        comment = NULL;
      }
      
      name_size = strlen (preset_name);
      comment_size = comment ? strlen(comment) : 0;
      blob_size = sizeof (size) + name_size +
          sizeof (tracks) + sizeof (params) +
          4 * params +
          sizeof (size) + comment_size;
      blob = g_malloc (blob_size);
      j = 0;
      memcpy(&blob[j], &name_size, 4); j+=4;
      memcpy(&blob[j], preset_name, name_size); j+=name_size;
      memcpy(&blob[j], &tracks, 4); j+=4;
      memcpy(&blob[j], &params, 4); j+=4;
      memcpy(&blob[j], data, 4*params); j+=4*params;
      memcpy(&blob[j], &comment_size, 4); j+=4;
      if(comment) {
        memcpy(&blob[j], comment, comment_size); j+=comment_size;
      }
      if ((pos = scan_data(garbage_path, blob, blob_size)) > -1) {
        printf ("    FOUND AT OFFSET %ld\n", pos);
      }      

      g_free (blob);
      g_free (comment);
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
