/*
 * FOS fosmount: FUSE extension in read-only for Smaky file system
 * Copyright (C) 2006-2007 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
 *
 * Thanks to Pierre Arnaud <pierre.arnaud@opac.ch>
 *           for his help and the documentation
 *    And to Epsitec SA for the Smaky computers
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>     /* strcmp strncmp strstr strlen strdup memcpy memset */
#include <time.h>       /* mktime */
#include <fuse.h>

#include "fosfat.h"

/* Rights */
#define FOS_DIR             0555
#define FOS_FILE            0444


static FOSFAT_DEV *dev;


/** Convert 's_fosfat_file' to 'struct stat'.
 * @param file file description
 * @return the stat
 */
static struct stat *in_stat(s_fosfat_file *file) {
  struct stat *st;
  struct tm time;

  st = (struct stat *)malloc(sizeof(struct stat));
  memset(st, 0, sizeof(*st));
  memset(&time, 0, sizeof(time));

  /* Directory or file */
  if (file->att.isdir) {
    st->st_mode = S_IFDIR | FOS_DIR;
    st->st_nlink = 2;
  }
  else {
    st->st_mode = S_IFREG | FOS_FILE;
    st->st_nlink = 1;
  }
  /* Size */
  st->st_size = file->size;
  /* Time */
  time.tm_year = file->time_r.year - 1900;
  time.tm_mon = file->time_r.month - 1;
  time.tm_mday = file->time_r.day;
  time.tm_hour = file->time_r.hour;
  time.tm_min = file->time_r.minute;
  time.tm_sec = file->time_r.second;
  st->st_atime = mktime(&time);
  time.tm_year = file->time_w.year - 1900;
  time.tm_mon = file->time_w.month - 1;
  time.tm_mday = file->time_w.day;
  time.tm_hour = file->time_w.hour;
  time.tm_min = file->time_w.minute;
  time.tm_sec = file->time_w.second;
  st->st_mtime = mktime(&time);
  time.tm_year = file->time_c.year - 1900;
  time.tm_mon = file->time_c.month - 1;
  time.tm_mday = file->time_c.day;
  time.tm_hour = file->time_c.hour;
  time.tm_min = file->time_c.minute;
  time.tm_sec = file->time_c.second;
  st->st_ctime = mktime(&time);

  return st;
}

/** Get the file stat from a path an return as struct stat.
 * @param path (/foo/bar)
 * @return the stat
 */
static struct stat *get_stat(const char *path) {
  struct stat *st = NULL;
  s_fosfat_file *file;

  if ((file = fosfat_get_stat(dev, path))) {
    st = in_stat(file);
    free(file);
  }
  return st;
}

/** FUSE : get attributes of a file.
 * @param path (foo/bar)
 * @param stbuf attributes
 * @return 0 for success
 */
static int fos_getattr(const char *path, struct stat *stbuf) {
  char *location;
  struct stat *st;

  /* Root directory */
  if (!strcmp(path, "/"))
    location = strdup("/sys_list");
  else
    location = (char *)path;

  /* Get file stats */
  if ((st = get_stat(location))) {
    memcpy(stbuf, st, sizeof(*stbuf));
    free(st);
  }
  else
    return -ENOENT;

  if (location != path)
    free(location);

  return 0;
}

/** FUSE : read a directory.
 * @param path (foo/bar)
 * @param buf buffer for filler
 * @param filler function for put each entry
 * @param offset not used
 * @param fi not used
 * @return 0 for success
 */
static int fos_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
  s_fosfat_file *files, *first_file;

  (void)offset;
  (void)fi;

  /* First entries */
  filler(buf, "..", NULL, 0);

  /* Files and directories */
  if ((files = fosfat_list_dir(dev, path))) {
    first_file = files;
    do {
      struct stat *st = in_stat(files);
      char *name = strdup(files->name);
      if (strstr(name, ".dir"))
        *(name + strlen(name) - 4) = '\0';
      /* Add entry in the file list */
      filler(buf, name, st, 0);
      free(st);
      free(name);
    } while ((files = files->next_file));
    fosfat_free_listdir(first_file);
  }
  else
    return -ENOENT;
  return 0;
}

/** FUSE : test if a file can be opened.
 * @param path (foo/bar)
 * @param fi flags
 * @return 0 for success
 */
static int fos_open(const char *path, struct fuse_file_info *fi) {
  (void)path;

  if((fi->flags & 3) != O_RDONLY)
    return -EACCES;

  if(!fosfat_p_isopenexm(dev, path))
    return -ENOENT;

  return 0;
}

/** FUSE : read the data of a file.
 * @param path (foo/bar)
 * @param buf buffer for put the data
 * @param size size in bytes
 * @param offset offset un bytes
 * @param fi not used
 * @return the size
 */
static int fos_read(const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi)
{
  int length;
  char *buf_tmp;
  s_fosfat_file *file;

  (void)fi;

  /* Get the stats and test if it is a file */
  if ((file = fosfat_get_stat(dev, path)) && !file->att.isdir) {
    length = file->size;
    if (offset < length) {
      /* Fix the size in function of the offset */
      if (offset + (signed)size > length)
        size = length - offset;
      /* Read the data */
      buf_tmp = fosfat_get_buffer(dev, path, offset, size);
      /* Copy the data for FUSE */
      if (buf_tmp) {
        memcpy(buf, buf_tmp, size);
        free(buf_tmp);
      }
      else
        return -ENOENT;
    }
    else
      size = 0;
  }
  else
    return -ENOENT;
  return size;
}

/** Print help. */
void print_info(void) {
  printf("Usage: fosmount device mountpoint [--harddisk] [--debug]\n");
  printf("FUSE extension for a read-only access on Smaky FOS. Fosfat-%s\n\n",
         VERSION);
  printf("device       /dev/fd0 : floppy disk\n");
  printf("             /dev/sda : hard disk, etc\n\n");
  printf("mountpoint   for example, /mnt/smaky\n\n");
  printf("--harddisk   if you use an hard disk and not a floppy,\n");
  printf("             use this option\n\n");
  printf("--debug      that will turn on the fuse debugger\n\n");
  printf("\nPlease, report bugs to <fosfat-devel@gamesover.ch>\n");
}

/** FUSE implemented functions */
static struct fuse_operations fosfat_oper = {
  .getattr = fos_getattr,
  .readdir = fos_readdir,
  .open    = fos_open,
  .read    = fos_read,
};

int main(int argc, char **argv) {
  int i;
  int res = 0, debug = 0;
  char *device;
  char **arg;
  e_fosfat_disk type;

  /* Default type is Floppy Disk */
  type = eFD;

  /* Check arguments */
  if (argc < 3) {
    print_info();
    return -1;
  }
  else {
    for (i = 3; argc > 3 && i < argc; i++) {
      if (!strcmp(argv[i], "--harddisk"))
        type = eHD;
      else if (!strcmp(argv[i], "--debug"))
        debug = 1;
    }
    arg = (char **)malloc(sizeof(char *) * (2 + debug));
    if (arg) {
      arg[0] = strdup(argv[0]);
      if (debug)
        arg[debug] = strdup("-d");
      arg[1 + debug] = strdup(argv[2]);
      device = strdup(argv[1]);
    }
    else
      return -1;
  }

  /* Open the floppy disk (or hard disk) */
  if (!(dev = fosfat_opendev(device, type))) {
    printf("Could not open %s for mounting!\n", device);
    res = -1;
  }
  else {
    /* FUSE */
    res = fuse_main(2 + debug, arg, &fosfat_oper);

    /* Close the device */
    fosfat_closedev(dev);
  }

  /* Free */
  for (i = 0; i < 2 + debug; i++)
    free(arg[i]);
  free(arg);
  free(device);

  return res;
}
