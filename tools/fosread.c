/*
 * FOS fosread: tool in read-only for Smaky file system
 * Copyright (C) 2006-2008,2025 Mathieu Schroeter <mathieu@schroetersa.ch>
 *
 * Thanks to Pierre Arnaud for his help and the documentation
 *    And to Epsitec SA for the Smaky computers
 *
 * This file is part of Fosfat.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* strcmp strcpy */
#include <getopt.h>
#include <sys/stat.h> /* mkdir */

#include "fosfat.h"
#include "fosgra.h"


typedef struct ginfo {
  char name[FOSFAT_NAMELGT];
} global_info_t;

#ifdef _WIN32
#define HELP_DEVICE \
"file.di : disk image\n" \
"                       a       : floppy disk\n" \
"                       c       : hard disk, etc\n"
#else
#undef HELP_DEVICE
#define HELP_DEVICE \
"file.di  : disk image\n" \
"                       /dev/fd0 : floppy disk\n" \
"                       /dev/sda : hard disk, etc\n"
#endif

#define HELP_TEXT \
"Tool for a read-only access on a Smaky disk. fosfat-" LIBFOSFAT_VERSION_STR "\n\n" \
"Usage: fosread [options] device mode [node] [path]\n\n" \
" -h --help             this help\n" \
" -v --version          version\n" \
" -a --harddisk         force an hard disk (default autodetect)\n" \
" -f --floppydisk       force a floppy disk (default autodetect)\n" \
" -l --fos-logger       that will turn on the FOS logger\n" \
" -u --undelete         enable the undelete mode, even deleted files will\n" \
"                       be listed and sometimes restorable with 'get' mode.\n" \
" -i --image-bmp        convert .IMAGE and .COLOR to .BMP\n" \
" -t --text             convert some text files to .TXT\n\n" \
" device                " HELP_DEVICE \
" mode\n" \
"   list                list the content of a node\n" \
"   get                 copy a file (or a directory) from the Smaky's disk in\n" \
"                       a local directory\n" \
" node                  the tree with the file (or folder)" \
" for 'get' or 'list'\n" \
"                       example: foo/bar/toto.text\n" \
" path                  you can specify a path to save" \
" the file (with get mode)\n" \
"\nPlease, report bugs to <mathieu@schroetersa.ch>.\n"

#define VERSION_TEXT "fosread-" LIBFOSFAT_VERSION_STR "\n"


#define FLYID "@"

static int g_bmp = 0;
static int g_txt = 0;


static inline int
my_mkdir (const char *pathname)
{
#ifdef _WIN32
  return mkdir (pathname);
#else
  return mkdir (pathname, 0755);
#endif /* !_WIN32 */
}

/*
 * Get info from the disk.
 *
 * fosfat the main structure
 * return info
 */
static global_info_t *
get_ginfo (fosfat_t *fosfat)
{
  global_info_t *ginfo;
  char *name;

  name = fosfat_diskname (fosfat);
  if (name)
  {
    ginfo = malloc (sizeof (global_info_t));
    strncpy (ginfo->name, name, FOSFAT_NAMELGT - 1);
    ginfo->name[FOSFAT_NAMELGT - 1] = '\0';
    free (name);
  }
  else
  {
    fprintf (stderr, "ERROR: I can't read the name of this disk!\n");
    ginfo = NULL;
  }

  return ginfo;
}

/*
 * Print date and hour.
 *
 * time date and hour
 */
static void
print_date (fosfat_time_t *time)
{
  printf (" %4i-%02i-%02i %02i:%02i", time->year, time->month,
                                      time->day, time->hour, time->minute);
}

/*
 * Print a file in the list.
 *
 * file description
 */
static void
print_file (fosfat_file_t *file)
{
  char filename[FOSFAT_NAMELGT];

  if (file->att.isdir || file->att.islink)
    snprintf (filename, strrchr (file->name, '.') - file->name + 1,
              "%s", file->name);
  else
    strncpy (filename, file->name, FOSFAT_NAMELGT - 1);

  filename[FOSFAT_NAMELGT - 1] = '\0';

  printf ("%c%c%c %8i", file->att.isdir ? 'd' : (file->att.islink ? 'l' : '-'),
                        file->att.isvisible ? '-' : 'h',
                        file->att.isencoded ? 'e' : '-',
                        file->size);
  print_date (&file->time_c);
  print_date (&file->time_w);
  print_date (&file->time_r);
  printf (" %s", filename);

  if (file->att.isdel)
    printf (" (X)");

  printf ("\n");
}

/*
 * List the content of a directory.
 *
 * fosfat       handle
 * path         where in the tree
 * return true if it is ok
 */
static int
list_dir (fosfat_t *fosfat, const char *loc)
{
  char *path;
  fosfat_file_t *files, *first_file;

  if (strcmp (loc, "/") && fosfat_islink (fosfat, loc))
    path = fosfat_symlink (fosfat, loc);
  else
    path = strdup (loc);

  if ((files = fosfat_list_dir (fosfat, path)))
  {
    first_file = files;
    printf ("path: %s\n\n", path);
    printf ("        size creation         last change");
    printf ("      last view        filename\n");
    printf ("        ---- --------         -----------");
    printf ("      ---------        --------\n");

    do
    {
      print_file (files);
    } while ((files = files->next_file));

    printf ("\nd:directory  l:link  h:hidden  e:encoded    (X):undelete\n");
    fosfat_free_listdir (first_file);
  }
  else
  {
    free (path);
    fprintf (stderr, "ERROR: I can't found this path!\n");
    return 0;
  }

  free (path);

  return 1;
}

/*
 * Copy a file from the disk.
 *
 * fosfat       handle
 * path         where in the tree
 * dst          where in local
 * return true if it is ok
 */
static int
get_file (fosfat_t *fosfat, const char *path, const char *dst)
{
  int res = 0;
  char *new_file, *name = NULL;
  fosfat_ftype_t ftype = FOSFAT_FTYPE_OTHER;

  if (!strcasecmp (dst, "./"))
    new_file = strdup ((strrchr (path, '/') ? strrchr (path, '/') + 1 : path));
  else
    new_file = strdup (dst);

  if (!fosfat_islink (fosfat, path) && !fosfat_isdir (fosfat, path))
  {
    FILE *fp = NULL;
    uint8_t *buffer = NULL;
    size_t size = 0;
    int is_bmp = 0;
    int is_txt = 0;

    printf ("File \"%s\" is copying ...\n", path);
    ftype = fosfat_ftype (path);

    is_bmp = g_bmp && ftype == FOSFAT_FTYPE_IMAGE
                   && fosgra_is_image (fosfat, path);
    is_txt = g_txt && ftype == FOSFAT_FTYPE_TEXT;

    if (is_bmp)
    {
      name = calloc (1, strlen (new_file) + strlen (FLYID) + 6);
      sprintf (name, "%s." FLYID ".bmp", new_file);
    }
    else if (is_txt)
    {
      name = calloc (1, strlen (new_file) + strlen (FLYID) + 6);
      sprintf (name, "%s." FLYID ".txt", new_file);
    }
    else
      name = strdup (new_file);

    if (is_bmp || is_txt)
    {
      fp = fopen (name, "wb");
      if (!fp)
      {
        fprintf (stderr, "ERROR: I can't write file: %s\n", name);
        goto out;
      }
    }

    if (g_bmp && ftype == FOSFAT_FTYPE_IMAGE)
      buffer = fosgra_bmp_get_buffer (fosfat, path, &size);

    if (g_txt && ftype == FOSFAT_FTYPE_TEXT)
    {
      fosfat_file_t *file = fosfat_get_stat (fosfat, path);
      size = file->size;
      free (file);
      buffer = fosfat_get_buffer (fosfat, path, 0, size);
      fosfat_sma2iso8859 ((char *) buffer, size, FOSFAT_ASCII_LF);
    }

    if (fp && buffer)
    {
      size_t nb = fwrite (buffer, 1, size, fp);
      if (nb == size)
        res = 1;
      else
        fprintf (stderr, "ERROR: file is truncated: %s\n", name);
    }

    if (fp)
      fclose (fp);
    if (buffer)
      free (buffer);

    if (res == 0 && fosfat_get_file (fosfat, path, name, 1))
    {
      res = 1;
      printf ("Okay..\n");
    }
    else if (res == 0)
      fprintf (stderr, "ERROR: I can't copy the file!\n");
  }
  else
    fprintf (stderr, "ERROR: I can't copy a directory or a link!\n");

out:
  free (new_file);
  if (name)
    free (name);
  return res;
}

static int
get_dir (fosfat_t *fosfat, const char *loc, const char *dst)
{
  char *path;
  fosfat_file_t *file, *first_file;

  if (strcmp (loc, "/") && fosfat_islink (fosfat, loc))
    path = fosfat_symlink (fosfat, loc);
  else
    path = strdup (loc);

  if ((file = fosfat_list_dir (fosfat, path)))
  {
    char out[4096] = {0};
    char in[256]  = {0};
    first_file = file;

    do
    {
      if (file->att.islink)
        continue;

      if (file->name[0] == '.')
        continue;

      snprintf (in , sizeof (in),  "%s/%s", loc, file->name);
      snprintf (out, sizeof (out), "%s/%s", dst, file->name);

      if (file->att.isdir)
      {
        char *it = strrchr (out, '.');
        if (it)
          *it = '\0'; /* drop .dir from the name */
        my_mkdir (out);
        get_dir (fosfat, in, out);
      }
      else
      {
        if (file->size > 0)
          get_file (fosfat, in, out);
        else
          fprintf (stderr, "WARNING: skip empty file (0 bytes)\n");
      }
    } while ((file = file->next_file));

    fosfat_free_listdir (first_file);
  }
  else
  {
    free (path);
    fprintf (stderr, "ERROR: I can't found this path!\n");
    return 0;
  }

  free (path);

  return 1;
}

/* Print help. */
static void
print_info (void)
{
  printf (HELP_TEXT);
}

/* Print version. */
static void
print_version (void)
{
  printf (VERSION_TEXT);
}

int
main (int argc, char **argv)
{
  int res = 0, i, next_option, undelete = 0;
  int flags = 0;
  fosfat_disk_t type = FOSFAT_AD;
  char *device = NULL, *mode = NULL, *node = NULL, *path = NULL;
  fosfat_t *fosfat;
  global_info_t *ginfo = NULL;

  const char *const short_options = "afhluitv";

  const struct option long_options[] = {
    { "harddisk",     no_argument, NULL, 'a' },
    { "floppydisk",   no_argument, NULL, 'f' },
    { "help",         no_argument, NULL, 'h' },
    { "fos-logger",   no_argument, NULL, 'l' },
    { "undelete",     no_argument, NULL, 'u' },
    { "image-bmp",    no_argument, NULL, 'i' },
    { "text",         no_argument, NULL, 't' },
    { "version",      no_argument, NULL, 'v' },
    { NULL,           0,           NULL,  0  }
  };

  /* check options */
  do
  {
    next_option = getopt_long (argc, argv, short_options, long_options, NULL);
    switch (next_option)
    {
    default :           /* unknown */
    case '?':           /* invalid option */
    case 'h':           /* -h or --help */
      print_info ();
      return -1;
    case 'v':           /* -v or --version */
      print_version ();
      return -1;
    case 'a':           /* -a or --harddisk */
      type = FOSFAT_HD;
      break ;
    case 'f':           /* -f or --floppydisk */
      type = FOSFAT_FD;
      break ;
    case 'l':           /* -l or --fos-logger */
      fosfat_logger (1);
      break ;
    case 'u':           /* -u or --undelete */
      undelete = 1;
      break ;
    case 'i':           /* -i or --image-bmp */
      g_bmp = 1;
      break ;
    case 't':           /* -t or --text */
      g_txt = 1;
      break;
    case -1:            /* end */
      break ;
    }
  } while (next_option != -1);

  if (argc < optind + 2)
  {
    print_info ();
    return -1;
  }

  for (i = optind; i < argc; i++)
  {
    if (i == optind)
      device = strdup (argv[optind]);
    else if (i == optind + 1)
      mode = strdup (argv[optind + 1]);
    else if (i == optind + 2)
      node = strdup (argv[optind + 2]);
    else if (i == optind + 3)
      path = strdup (argv[optind + 3]);
  }

  if (undelete)
    flags = F_UNDELETE;

  /* Open the floppy disk (or hard disk) */
  if (!(fosfat = fosfat_open (device, type, flags)))
  {
    fprintf (stderr, "Could not open %s for reading!\n", device);
    res = -1;
  }

  /* Get globals informations on the disk */
  if (!res && (ginfo = get_ginfo (fosfat)))
  {
    printf ("Smaky disk %s\n", ginfo->name);

    /* Show the list of a directory */
    if (!strcasecmp (mode, "list"))
    {
      if (!node)
      {
        if (!list_dir (fosfat, "/"))
          res = -1;
      }
      else if (node)
      {
        if (!list_dir (fosfat, node))
          res = -1;
        free (node);
      }
    }

    /* Get a file from the disk */
    else if (!strcmp (mode, "get") && node)
    {
      if (fosfat_isdir (fosfat, node))
        get_dir (fosfat, node, path ? path : "./");
      else
        get_file (fosfat, node, path ? path : "./");
      free (node);
      free (path);
    }
    else
      print_info ();

    free (ginfo);
  }

  /* Close the disk */
  fosfat_close (fosfat);
  free (device);
  free (mode);

  return res;
}
