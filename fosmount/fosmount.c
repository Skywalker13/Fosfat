/*
 * FOS fosmount: FUSE extension in read-only for Smaky file system
 * Copyright (C) 2006-2007 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
 *
 * Thanks to Pierre Arnaud <pierre.arnaud@opac.ch>
 *           for its help and the documentation
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
" -d --fuse-debugger    that will turn on the FUSE debugger\n\n" \
" device                /dev/fd0 : floppy disk\n" \
"                       /dev/sda : hard disk, etc\n" \
" mountpoint            for example, /mnt/smaky\n" \
"\nPlease, report bugs to <fosfat-devel@gamesover.ch>.\n"

#define VERSION_TEXT "fosmount-" VERSION "\n"


static fosfat_t *fosfat;


/**
 * \brief Convert 'fosfat_file_t' to 'struct stat'.
 *
 * \param file file description
 * \return the stat
 */
static struct stat *in_stat(fosfat_file_t *file) {
  struct stat *st;
  struct tm time;

  st = (struct stat *)malloc(sizeof(struct stat));
  memset(st, 0, sizeof(*st));
  memset(&time, 0, sizeof(time));

  /* Directory, symlink or file */
  if (file->att.isdir) {
    st->st_mode = S_IFDIR | FOS_DIR;
    st->st_nlink = 2;
  }
  else if (file->att.islink) {
    st->st_mode = S_IFLNK | FOS_DIR;
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

/**
 * \brief Get the file stat from a path an return as struct stat.
 *
 * \param path (/foo/bar)
 * \return the stat
 */
static struct stat *get_stat(const char *path) {
  struct stat *st = NULL;
  fosfat_file_t *file;

  if ((file = fosfat_get_stat(fosfat, path))) {
    st = in_stat(file);
    free(file);
  }

  return st;
}

/**
 * \brief FUSE : read the target of a symlink.
 *
 * \param path (foo/bar)
 * \param dst target
 * \param size max length
 * \return 0 for success
 */
static int fos_readlink(const char *path, char *dst, size_t size) {
  int res = -ENOENT;
  char *link;

  link = fosfat_symlink(fosfat, path);

  if (strlen(link) < size) {
    memcpy(dst, link, strlen(link) + 1);
    res = 0;
  }

  if (link)
    free(link);

  return res;
}

/**
 * \brief FUSE : get attributes of a file.
 *
 * \param path (foo/bar)
 * \param stbuf attributes
 * \return 0 for success
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

/**
 * \brief FUSE : read a directory.
 *
 * \param path (foo/bar)
 * \param buf buffer for filler
 * \param filler function for put each entry
 * \param offset not used
 * \param fi not used
 * \return 0 for success
 */
static int fos_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
  fosfat_file_t *files, *first_file;

  (void)offset;
  (void)fi;

  /* First entries */
  filler(buf, "..", NULL, 0);

  /* Files and directories */
  if ((files = fosfat_list_dir(fosfat, path))) {
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

/**
 * \brief FUSE : test if a file can be opened.
 *
 * \param path (foo/bar)
 * \param fi flags
 * \return 0 for success
 */
static int fos_open(const char *path, struct fuse_file_info *fi) {
  (void)path;

  if((fi->flags & 3) != O_RDONLY)
    return -EACCES;

  if(!fosfat_isopenexm(fosfat, path))
    return -ENOENT;

  return 0;
}

/**
 * \brief FUSE : read the data of a file.
 *
 * \param path (foo/bar)
 * \param buf buffer for put the data
 * \param size size in bytes
 * \param offset offset un bytes
 * \param fi not used
 * \return the size
 */
static int fos_read(const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi)
{
  int length;
  char *buf_tmp;
  fosfat_file_t *file;

  (void)fi;

  /* Get the stats and test if it is a file */
  if ((file = fosfat_get_stat(fosfat, path))) {
    if (!file->att.isdir) {
      length = file->size;

      if (offset < length) {
        /* Fix the size in function of the offset */
        if (offset + (signed)size > length)
          size = length - offset;

        /* Read the data */
        buf_tmp = fosfat_get_buffer(fosfat, path, offset, size);

        /* Copy the data for FUSE */
        if (buf_tmp) {
          memcpy(buf, buf_tmp, size);
          free(buf_tmp);
        }
        else
          size = -ENOENT;
      }
      else
        size = 0;
    }
    free(file);
  }
  else
    size = -ENOENT;

  return size;
}

/** Print help. */
void print_info(void) {
  printf(HELP_TEXT);
}

/** Print version. */
void print_version(void) {
  printf(VERSION_TEXT);
}

/** FUSE implemented functions */
static struct fuse_operations fosfat_oper = {
  .getattr  = fos_getattr,
  .readdir  = fos_readdir,
  .open     = fos_open,
  .read     = fos_read,
  .readlink = fos_readlink,
};

int main(int argc, char **argv) {
  int i;
  int next_option;
  int res = 0, fusedebug = 0, foslog = 0;
  char *device;
  char **arg;
  fosfat_disk_t type = eDAUTO;

  const char *const short_options = "adfhlv";

  const struct option long_options[] = {
    { "harddisk",      0, NULL, 'a' },
    { "fuse-debugger", 0, NULL, 'd' },
    { "floppydisk",    0, NULL, 'f' },
    { "help",          0, NULL, 'h' },
    { "fos-logger",    0, NULL, 'l' },
    { "version",       0, NULL, 'v' },
    { NULL,            0, NULL,  0  }
  };

  /* check options */
  do {
    next_option = getopt_long(argc, argv, short_options, long_options, NULL);
    switch (next_option) {
      default :           /* unknown */
      case '?':           /* invalid option */
      case 'h':           /* -h or --help */
        print_info();
        return -1;
      case 'v':           /* -v or --version */
        print_version();
        return -1;
      case 'f':           /* -f or --floppydisk */
        type = eFD;
        break ;
      case 'a':           /* -a or --harddisk */
        type = eHD;
        break ;
      case 'd':           /* -d or --fuse-debugger */
        fusedebug = 1;
        break ;
      case 'l':           /* -l or --fos-debugger */
        foslog = 1;
        fosfat_logger(1);
        break ;
      case -1:            /* end */
        break ;
    }
  } while (next_option != -1);

  if (argc < optind + 2) {
    print_info();
    return -1;
  }

  /* table for fuse */
  arg = malloc(sizeof(char *) * (3 + fusedebug + foslog));
  if (arg) {
    arg[0] = strdup(argv[0]);

    if (fusedebug)
      arg[fusedebug] = strdup("-d");
    if (foslog)
      arg[fusedebug + foslog] = strdup("-f");

    device = strdup(argv[optind]);
    arg[1 + fusedebug + foslog] = strdup(argv[optind + 1]);

    /* FUSE must be used as single-thread */
    arg[2 + fusedebug + foslog] = strdup("-s");
  }
  else
    return -1;

  /* Open the floppy disk (or hard disk) */
  if (!(fosfat = fosfat_open(device, type))) {
    printf("Could not open %s for mounting!\n", device);
    res = -1;
  }
  else {
    /* FUSE */
    res = fuse_main(3 + fusedebug + foslog, arg, &fosfat_oper, NULL);

    for (i = 0; i < 3 + fusedebug + foslog; i++)
      free(*(arg + i));
    free(arg);

    /* Close the device */
    fosfat_close(fosfat);
  }

  /* Free */
  free(device);

  return res;
}
