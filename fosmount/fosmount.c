/*
 * FOS fosmount: FUSE extension in read-only for Smaky file system
 * Copyright (C) 2006-2010 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
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
#include <errno.h>
#include <fcntl.h>
#include <string.h>     /* strcmp strncmp strstr strlen strdup memcpy memset */
#include <time.h>       /* mktime */
#include <fuse.h>
#include <getopt.h>

#include "fosfat.h"
#include "fosgra.h"

/* Rights */
#define FOS_DIR             0555
#define FOS_FILE            0444

#define HELP_TEXT \
"FUSE extension for a read-only access on Smaky FOS. fosfat-" VERSION "\n\n" \
"Usage: fosmount [options] device mountpoint\n\n" \
" -h --help             this help\n" \
" -v --version          version\n" \
" -a --harddisk         force an hard disk (default autodetect)\n" \
" -f --floppydisk       force a floppy disk (default autodetect)\n" \
" -l --fos-logger       that will turn on the FOS logger\n" \
" -d --fuse-debugger    that will turn on the FUSE debugger\n" \
" -i --image-pbm        convert on the fly .IMAGE to .PBM\n" \
" -j --image-xpm        convert on the fly .COLOR to .XPM\n\n" \
" device                /dev/fd0 : floppy disk\n" \
"                       /dev/sda : hard disk, etc\n" \
" mountpoint            for example, /mnt/smaky\n" \
"\nPlease, report bugs to <fosfat-devel@gamesover.ch>.\n"

#define VERSION_TEXT "fosmount-" VERSION "\n"


static fosfat_t *fosfat;

#define FOSID_PBM "[fosgra]"
#define FOSID_XPM "[fosgra]"

static int g_pbm = 0;
static int g_xpm = 0;


static int
count_nb_dec (int dec)
{
  int size = 1;

  while (dec /= 10)
    size++;

  return size;
}

static char *
trim_fosname (const char *path)
{
  char *it, res[256];

  snprintf (res, sizeof (res), "%s", path);
  it = strstr (res, "." FOSID_PBM ".");
  if (it)
    *it = '\0';

  return strdup (res);
}

static size_t
get_filesize (fosfat_file_t *file, const char *path)
{
  char *it = NULL;
  uint8_t bpp = 0;
  uint16_t x = 0, y = 0;

  if (g_pbm)
    it = strstr (file->name, ".image\0");
  if (!it && g_xpm)
    it = strstr (file->name, ".color\0");

  if (!it || file->att.isdir || file->att.islink || file->att.isencoded
      || !fosgra_is_image (fosfat, path))
    return file->size;

  fosgra_get_info (fosfat, path, &x, &y, &bpp);

  if (bpp == 4)
    return x * y + y + count_nb_dec (x) + count_nb_dec (y) + 14 + 16 * 12;

  return x / 8 * y + count_nb_dec (x) + count_nb_dec (y) + 5;
}

#define TO_8BPP(c) (((c) & 0x0F) + 0x61)   /* 'a' to 'p' */
#define COLOR_TO_XPM(dec, size, mod, sn)                            \
  {                                                                 \
    size_t s;                                                       \
    int j = 0, k = mod;                                             \
    for (s = 0; s < (size); s++)                                    \
    {                                                               \
      (dec)[s] = TO_8BPP ((buf[j] & (0xF0 >> 4 * k)) >> 4 * !k);    \
      k = !k;                                                       \
      if (!k)                                                       \
        j++;                                                        \
                                                                    \
      sn++;                                                         \
      if (sn && !(sn % x) && (s + 1) < (size))                      \
        (dec)[++s] = '\n';                                          \
    }                                                               \
  }

static uint8_t *
get_buffer (fosfat_file_t *file, const char *path, off_t offset, size_t size)
{
  char *it = NULL;
  char head[1024];
  char mapping[512];
  uint8_t *buf, *dec;
  uint8_t bpp = 0;
  uint16_t x = 0, y = 0;

  if (g_pbm)
    it = strstr (file->name, ".image\0");
  if (!it && g_xpm)
    it = strstr (file->name, ".color\0");

  if (!it || file->att.isdir || file->att.islink || file->att.isencoded
      || !fosgra_is_image (fosfat, path))
    return fosfat_get_buffer (fosfat, path, offset, size);

  fosgra_get_info (fosfat, path, &x, &y, &bpp);
  if (bpp == 1)
    snprintf (head, sizeof (head), "P4\n%u %u\n", x, y);
  else
  {
    int i;

    snprintf (head, sizeof (head), "! XPM2\n%u %u 16 1\n", x, y);
    for (i = 0; i < 16; i++)
    {
      snprintf (mapping, sizeof (mapping), "%c c #%06x\n",
                TO_8BPP (i), fosgra_color_get (fosfat, path, i));
      strcat (head, mapping);
    }
  }

  /* no header needed */
  if (offset > (signed) strlen (head))
  {
    off_t off_conv   = offset - strlen (head);
    off_t off_n      = off_conv - off_conv / (x + 1);
    size_t size_conv = size;

    int start_n = off_conv % (x + 1);

    if (bpp == 4)
    {
      off_conv  = off_n / 2;
      size_conv = (size_conv - size_conv / (x + 1)) / 2 + 1;
    }

    buf = fosgra_get_buffer (fosfat, path, off_conv, size_conv);
    if (!buf)
      return NULL;

    if (bpp == 4)
    {
      dec = calloc (1, size);
      if (!dec)
      {
        free (buf);
        return NULL;
      }

      COLOR_TO_XPM (dec, size, off_n % 2, start_n)
      free (buf);
    }
    else
      dec = buf;
  }
  /* header for PBM / XPM */
  else
  {
    off_t off_conv   = 0;
    size_t size_conv = size - (strlen (head) - offset);

    int start_n = 0;

    if (bpp == 4)
      size_conv = (size_conv - size_conv / (x + 1)) / 2 + 1;

    buf = fosgra_get_buffer (fosfat, path, off_conv, size_conv);
    if (!buf)
      return NULL;

    dec = calloc (1, size);
    if (!dec)
      return NULL;

    memcpy (dec, head + offset, strlen (head) - offset); /* copy header */

    if (bpp == 4)
      COLOR_TO_XPM (dec + (strlen (head) - offset),
                    size - (strlen (head) - offset), 0, start_n)
    else
      memcpy (dec + (strlen (head) - offset), /* copy data */
              buf, size - (strlen (head) - offset));
    free (buf);
  }
  return dec;
}

/*
 * Convert 'fosfat_file_t' to 'struct stat'.
 */
static struct stat *
in_stat (fosfat_file_t *file, const char *path)
{
  struct stat *st;
  struct tm time;

  st = (struct stat *) malloc (sizeof (struct stat));
  memset (st, 0, sizeof (*st));
  memset (&time, 0, sizeof (time));

  /* Directory, symlink or file */
  if (file->att.isdir)
  {
    st->st_mode = S_IFDIR | FOS_DIR;
    st->st_nlink = 2;
  }
  else if (file->att.islink)
  {
    st->st_mode = S_IFLNK | FOS_DIR;
    st->st_nlink = 2;
  }
  else
  {
    st->st_mode = S_IFREG | FOS_FILE;
    st->st_nlink = 1;
  }

  /* Size */
  st->st_size = get_filesize (file, path);

  /* Time */
  time.tm_year = file->time_r.year - 1900;
  time.tm_mon  = file->time_r.month - 1;
  time.tm_mday = file->time_r.day;
  time.tm_hour = file->time_r.hour;
  time.tm_min  = file->time_r.minute;
  time.tm_sec  = file->time_r.second;
  st->st_atime = mktime(&time);
  time.tm_year = file->time_w.year - 1900;
  time.tm_mon  = file->time_w.month - 1;
  time.tm_mday = file->time_w.day;
  time.tm_hour = file->time_w.hour;
  time.tm_min  = file->time_w.minute;
  time.tm_sec  = file->time_w.second;
  st->st_mtime = mktime(&time);
  time.tm_year = file->time_c.year - 1900;
  time.tm_mon  = file->time_c.month - 1;
  time.tm_mday = file->time_c.day;
  time.tm_hour = file->time_c.hour;
  time.tm_min  = file->time_c.minute;
  time.tm_sec  = file->time_c.second;
  st->st_ctime = mktime(&time);

  return st;
}

/*
 * Get the file stat from a path an return as struct stat.
 */
static struct stat *
get_stat (const char *path)
{
  char *location;
  struct stat *st = NULL;
  fosfat_file_t *file;

  location = trim_fosname (path);

  if ((file = fosfat_get_stat (fosfat, location)))
  {
    st = in_stat (file, location);
    free (file);
  }

  if (location)
    free (location);

  return st;
}

/*
 * FUSE : read the target of a symlink.
 *
 * path         (foo/bar)
 * dst          target
 * size         max length
 * return 0 for success
 */
static int
fos_readlink (const char *path, char *dst, size_t size)
{
  int res = -ENOENT;
  char *link, *location;

  location = trim_fosname (path);

  link = fosfat_symlink (fosfat, location);

  if (strlen (link) < size)
  {
    memcpy (dst, link, strlen (link) + 1);
    res = 0;
  }

  if (link)
    free (link);
  if (location)
    free (location);

  return res;
}

/*
 * FUSE : get attributes of a file.
 *
 * path         (foo/bar)
 * stbuf        attributes
 * return 0 for success
 */
static int
fos_getattr (const char *path, struct stat *stbuf)
{
  int ret = 0;
  char *location;
  struct stat *st;

  /* Root directory */
  if (!strcmp (path, "/"))
    location = strdup ("/sys_list");
  else
    location = trim_fosname (path);

  /* Get file stats */
  if ((st = get_stat (location)))
  {
    memcpy (stbuf, st, sizeof (*stbuf));
    free (st);
  }
  else
    ret = -ENOENT;

  if (location)
    free (location);

  return ret;
}

/*
 * FUSE : read a directory.
 *
 * path         (foo/bar)
 * buf          buffer for filler
 * filler       function for put each entry
 * offset       not used
 * fi           not used
 * return 0 for success
 */
static int
fos_readdir (const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi)
{
  int ret = 0;
  char *location;
  fosfat_file_t *files, *first_file;

  (void) offset;
  (void) fi;

  location = trim_fosname (path);

  /* First entries */
  filler (buf, "..", NULL, 0);

  /* Files and directories */
  if ((files = fosfat_list_dir (fosfat, location)))
  {
    first_file = files;

    do
    {
      char _path[256];
      struct stat *st;
      char *name;

      snprintf (_path, sizeof (_path), "%s/%s", location, files->name);
      st = in_stat (files, _path);

      /* add identification for .IMAGE translated to PBM */
      if (g_pbm && strstr (files->name, ".image\0")
          && fosgra_is_image (fosfat, _path))
      {
        name = calloc (1, strlen (files->name) + strlen (FOSID_PBM) + 6);
        sprintf (name, "%s." FOSID_PBM ".pbm", files->name);
      }
      /* add identification for .COLOR translated to XPM */
      else if (g_xpm && strstr (files->name, ".color\0")
               && fosgra_is_image (fosfat, _path))
      {
        name = calloc (1, strlen (files->name) + strlen (FOSID_XPM) + 6);
        sprintf (name, "%s." FOSID_XPM ".xpm", files->name);
      }
      else
        name = strdup (files->name);

      if (strstr (name, ".dir"))
        *(name + strlen (name) - 4) = '\0';

      /* Add entry in the file list */
      filler (buf, name, st, 0);
      free (st);
      free (name);
    } while ((files = files->next_file));
    fosfat_free_listdir (first_file);
  }
  else
    ret = -ENOENT;

  if (location)
    free (location);

  return ret;
}

/*
 * FUSE : test if a file can be opened.
 *
 * path         (foo/bar)
 * fi           flags
 * return 0 for success
 */
static int
fos_open (const char *path, struct fuse_file_info *fi)
{
  int ret = 0;
  char *location;

  location = trim_fosname (path);

  if ((fi->flags & 3) != O_RDONLY)
    ret = -EACCES;
  else if (!fosfat_isopenexm (fosfat, location))
    ret = -ENOENT;

  if (location)
    free (location);

  return ret;
}

/*
 * FUSE : read the data of a file.
 *
 * path         (foo/bar)
 * buf          buffer for put the data
 * size         size in bytes
 * offset       offset in bytes
 * fi           not used
 * return the size
 */
static int
fos_read (const char *path, char *buf, size_t size,
          off_t offset, struct fuse_file_info *fi)
{
  int length;
  char *location;
  uint8_t *buf_tmp;
  fosfat_file_t *file;

  (void) fi;

  location = trim_fosname (path);

  /* Get the stats and test if it is a file */
  if ((file = fosfat_get_stat (fosfat, location)))
  {
    if (!file->att.isdir)
    {
      length = get_filesize (file, location);

      if (offset < length)
      {
        /* Fix the size in function of the offset */
        if (offset + (signed) size > length)
          size = length - offset;

        /* Read the data */
        buf_tmp = get_buffer (file, location, offset, size);

        /* Copy the data for FUSE */
        if (buf_tmp)
        {
          memcpy (buf, buf_tmp, size);
          free (buf_tmp);
        }
        else
          size = -ENOENT;
      }
      else
        size = 0;
    }
    free (file);
  }
  else
    size = -ENOENT;

  if (location)
    free (location);

  return size;
}

/* Print help. */
void
print_info (void)
{
  printf (HELP_TEXT);
}

/* Print version. */
void
print_version (void)
{
  printf (VERSION_TEXT);
}

/* FUSE implemented functions */
static struct fuse_operations fosfat_oper = {
  .getattr  = fos_getattr,
  .readdir  = fos_readdir,
  .open     = fos_open,
  .read     = fos_read,
  .readlink = fos_readlink,
};

int
main (int argc, char **argv)
{
  int i;
  int next_option;
  int res = 0, fusedebug = 0, foslog = 0;
  char *device;
  char **arg;
  fosfat_disk_t type = FOSFAT_AD;

  const char *const short_options = "adfhlijv";

  const struct option long_options[] = {
    { "harddisk",      no_argument, NULL, 'a' },
    { "fuse-debugger", no_argument, NULL, 'd' },
    { "floppydisk",    no_argument, NULL, 'f' },
    { "help",          no_argument, NULL, 'h' },
    { "fos-logger",    no_argument, NULL, 'l' },
    { "image-pbm",     no_argument, NULL, 'i' },
    { "image-xpm",     no_argument, NULL, 'j' },
    { "version",       no_argument, NULL, 'v' },
    { NULL,            0,           NULL,  0  }
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
    case 'f':           /* -f or --floppydisk */
      type = FOSFAT_FD;
      break ;
    case 'a':           /* -a or --harddisk */
      type = FOSFAT_HD;
      break ;
    case 'd':           /* -d or --fuse-debugger */
      fusedebug = 1;
      break ;
    case 'l':           /* -l or --fos-debugger */
      foslog = 1;
      fosfat_logger (1);
      break;
    case 'i':           /* -i or --image-pbm */
      g_pbm = 1;
      break ;
    case 'j':           /* -j or --image-xpm */
      g_xpm = 1;
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

  /* table for fuse */
  arg = malloc (sizeof (char *) * (3 + fusedebug + foslog));
  if (arg)
  {
    arg[0] = strdup (argv[0]);

    if (fusedebug)
      arg[fusedebug] = strdup ("-d");
    if (foslog)
      arg[fusedebug + foslog] = strdup ("-f");

    device = strdup (argv[optind]);
    arg[1 + fusedebug + foslog] = strdup (argv[optind + 1]);

    /* FUSE must be used as single-thread */
    arg[2 + fusedebug + foslog] = strdup ("-s");
  }
  else
    return -1;

  /* Open the floppy disk (or hard disk) */
  if (!(fosfat = fosfat_open (device, type, 0)))
  {
    fprintf (stderr, "Could not open %s for mounting!\n", device);
    res = -1;
  }
  else
  {
    /* FUSE */
    res = fuse_main (3 + fusedebug + foslog, arg, &fosfat_oper, NULL);

    /* Close the device */
    fosfat_close (fosfat);
  }

  for (i = 0; i < 3 + fusedebug + foslog; i++)
    free (*(arg + i));
  free (arg);

  /* Free */
  free (device);

  return res;
}
