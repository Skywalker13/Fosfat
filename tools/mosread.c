/*
 * SAMOS mosread: tool in read-only for Smaky file system
 * Copyright (C) 2025 Mathieu Schroeter <mathieu@schroetersa.ch>
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


#undef HELP_DEVICE
#define HELP_DEVICE \
"file.di  : disk image\n" \
"                       /dev/fd0 : floppy disk\n" \
"                       /dev/sda : hard disk, etc\n"

#define HELP_TEXT \
"Tool for a read-only access on a Smaky disk. fosfat-" LIBFOSFAT_VERSION_STR "\n\n" \
"Usage: fosread [options] device mode [node] [path]\n\n" \
" -h --help             this help\n" \
" -v --version          version\n" \
" -l --fos-logger       that will turn on the FOS logger\n\n" \
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

#define VERSION_TEXT "mosread-" LIBFOSFAT_VERSION_STR "\n"


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
 * Print date and hour.
 *
 * time date and hour
 */
static void
print_date (mosfat_time_t *time)
{
  printf (" %02u-%02u-%02u", time->year, time->month, time->day);
}

/*
 * Print a file in the list.
 *
 * file description
 */
static void
print_file (mosfat_file_t *file)
{
  char filename[MOSFAT_NAMELGT];

  strncpy (filename, file->name, MOSFAT_NAMELGT - 1);
  filename[MOSFAT_NAMELGT - 1] = '\0';

  printf ("%c %8i", file->att.isdir ? 'd' : '-', file->size);
  print_date (&file->time);
  printf (" %s", filename);
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
list_dir (mosfat_t *mosfat, const char *loc)
{
  char *path;
  mosfat_file_t *files, *first_file;

  path = strdup (loc);

  if ((files = mosfat_list_dir (mosfat, path)))
  {
    first_file = files;
    printf ("path: %s\n\n", path);
    printf ("      size date     filename\n");
    printf ("      ---- -------- --------\n");

    do
    {
      print_file (files);
    } while ((files = files->next_file));

    printf ("\nd:directory\n");
    mosfat_free_listdir (first_file);
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
get_file (mosfat_t *mosfat, const char *path, const char *dst)
{
  int res = 0;
  char *new_file, *name = NULL;
  fosfat_ftype_t ftype = FOSFAT_FTYPE_OTHER;

  if (!strcasecmp (dst, "./"))
    new_file = strdup ((strrchr (path, '/') ? strrchr (path, '/') + 1 : path));
  else
    new_file = strdup (dst);

  if (!mosfat_isdir (mosfat, path))
  {
    FILE *fp = NULL;
    uint8_t *buffer = NULL;
    size_t size = 0;

    printf ("File \"%s\" is copying ...\n", path);
    ftype = fosfat_ftype (path);
    name = strdup (new_file);

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

    /*if (res == 0 && mosfat_get_file (mosfat, path, name, 1))
    {
      res = 1;
      printf ("Okay..\n");
    }
    else if (res == 0)*/
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
get_dir (mosfat_t *mosfat, const char *loc, const char *dst)
{
  char *path;
  mosfat_file_t *file, *first_file;

  path = strdup (loc);

  if ((file = mosfat_list_dir (mosfat, path)))
  {
    char out[4096] = {0};
    char in[256]  = {0};
    first_file = file;

    do
    {
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
        get_dir (mosfat, in, out);
      }
      else
      {
        if (file->size > 0)
          get_file (mosfat, in, out);
        else
          fprintf (stderr, "WARNING: skip empty file (0 bytes)\n");
      }
    } while ((file = file->next_file));

    mosfat_free_listdir (first_file);
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
  int res = 0, i, next_option;
  char *device = NULL, *mode = NULL, *node = NULL, *path = NULL;
  mosfat_t *mosfat;

  const char *const short_options = "hlv";

  const struct option long_options[] = {
    { "help",         no_argument, NULL, 'h' },
    { "fos-logger",   no_argument, NULL, 'l' },
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
    case 'l':           /* -l or --fos-logger */
      fosfat_logger (1);
      break ;
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

  /* Open the image disk */
  if (!(mosfat = mosfat_open (device)))
  {
    fprintf (stderr, "Could not open %s for reading!\n", device);
    res = -1;
  }

  /* Get globals informations on the disk */
  if (!res)
  {
    /* Show the list of a directory */
    if (!strcasecmp (mode, "list"))
    {
      if (!node)
      {
        if (!list_dir (mosfat, "/"))
          res = -1;
      }
      else if (node)
      {
        if (!list_dir (mosfat, node))
          res = -1;
        free (node);
      }
    }

    /* Get a file from the disk */
    else if (!strcmp (mode, "get") && node)
    {
      if (mosfat_isdir (mosfat, node))
        get_dir (mosfat, node, path ? path : "./");
      else
        get_file (mosfat, node, path ? path : "./");
      free (node);
      free (path);
    }
    else
      print_info ();
  }

  /* Close the disk */
  mosfat_close (mosfat);
  free (device);
  free (mode);

  return res;
}
