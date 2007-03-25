/*
 * FOS fosread: tool in read-only for Smaky file system
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
#include <string.h> /* strcmp strcpy */

#include "fosfat.h"

typedef struct ginfo {
  char name[FOSFAT_NAMELGT];
} s_global_info;

/** Get info from the disk.
 * @param dev pointer on the device
 * @return info
 */
s_global_info *get_ginfo(FOSFAT_DEV *dev) {
  s_global_info *ginfo;
  char *name;

  if ((name = fosfat_diskname(dev))) {
    ginfo = (s_global_info *)malloc(sizeof(s_global_info));
    strncpy(ginfo->name, name, FOSFAT_NAMELGT - 1);
    ginfo->name[FOSFAT_NAMELGT - 1] = '\0';
    free(name);
  }
  else {
    printf("ERROR: I can't read the name of this disk!\n");
    ginfo = NULL;
  }
  return ginfo;
}

/** Print date and hour.
 * @param time date and hour
 */
void print_date(s_fosfat_time *time) {
  printf(" %4i-%02i-%02i %02i:%02i", time->year, time->month,
                                     time->day, time->hour, time->minute);
}

/** Print a file in the list.
 * @param location where
 * @param file description
 */
void print_file(const char *location, s_fosfat_file *file) {
  char *path;
  char filename[FOSFAT_NAMELGT];

  path = (char *)malloc(sizeof(location) + sizeof(file->name) + sizeof(char));

  if (*location == '\0' || !strcmp(location, "/"))
    strcpy(path, file->name);
  else
    sprintf(path, "%s/%s", location, file->name);

  if (file->att.isdir)
    snprintf(filename, strrchr(file->name, '.') - file->name + 1,
             "%s", file->name);
  else
    strncpy(filename, file->name, FOSFAT_NAMELGT - 1);
  filename[FOSFAT_NAMELGT - 1] = '\0';

  printf("%c%c%c %11i", file->att.isdir ? 'd' : '-',
                        file->att.isvisible ? '-' : 'h',
                        file->att.isencoded ? 'e' : '-',
                        file->size);
  print_date(&file->time_c);
  print_date(&file->time_w);
  print_date(&file->time_r);
  printf(" %s\n", filename);
  free(path);
}

/** List the content of a directory.
 * @param dev pointer on the device
 * @param path where in the tree
 * @return true if it is ok
 */
int list_dir(FOSFAT_DEV *dev, const char *path) {
  s_fosfat_file *files, *first_file;

  if ((files = fosfat_list_dir(dev, path))) {
    first_file = files;
    printf("path: %s\n\n", path);
    printf("           size creation         last change");
    printf("      last view        filename\n");
    printf("           ---- --------         -----------");
    printf("      ---------        --------\n");
    do {
      print_file(path, files);
    } while ((files = files->next_file));
    printf("\nd:directory  h:hidden  e:encoded\n");
    fosfat_free_listdir(first_file);
  }
  else {
    printf("ERROR: I can't found this path!\n");
    return 0;
  }
  return 1;
}

/** Copy a file from the disk.
 * @param dev pointer on the device
 * @param path where in the tree
 * @param dst where in local
 * @return true if it is ok
 */
int get_file(FOSFAT_DEV *dev, const char *path, const char *dst) {
  int res = 0;
  char *new_file;

  if (!strcasecmp(dst, "./"))
    new_file = strdup((strrchr(path, '/') ? strrchr(path, '/') + 1 : path));
  else
    new_file = strdup(dst);
  printf("File \"%s\" is copying ...\n", path);
  if (fosfat_get_file(dev, path, new_file, 1)) {
    res = 1;
    printf("Okay..\n");
  }
  else
    printf("ERROR: I can't copy the file!\n");

  free(new_file);
  return res;
}

/** Print help. */
void print_info(void) {
  printf("Usage: fosread device mode [node] [path] [--harddisk]\n");
  printf("Tool for a read-only access on a Smaky disk. Fosfat-%s\n\n", VERSION);
  printf("  device       for example, /dev/fd0\n\n");
  printf("  mode\n");
  printf("   list        list the content of a node\n");
  printf("   get         copy a file from the Smaky's disk in a\n");
  printf("               local directory\n\n");
  printf("  node         the tree with the file (or folder)\n");
  printf("               for a 'get' or a 'list'\n");
  printf("               example: foo/bar/toto.text\n\n");
  printf("  path         you can specify a path for save\n");
  printf("               the file (with get mode)\n\n");
  printf("  --harddisk   if you use an hard disk and not a floppy,\n");
  printf("               use this option\n\n");
  printf("\nPlease, report bugs to <fosfat-devel@gamesover.ch>\n");
}

int main(int argc, char **argv) {
  int res = 0;
  int harddisk = eFD;
  char *path, *choice, *file, *dst;
  FOSFAT_DEV *dev;
  s_global_info *ginfo;

  if (argc < 3) {
    print_info();
    return -1;
  }

  path = strdup(argv[1]);
  choice = strdup(argv[2]);
  harddisk = strcmp(argv[argc - 1], "--harddisk") ? eFD : eHD;

  /* Open the floppy disk (or hard disk) */
  if (!(dev = fosfat_opendev(path, harddisk)))
  {
    printf("Could not open %s for reading!\n", path);
    return -1;
  }

  /* Get globals informations on the disk */
  if ((ginfo = get_ginfo(dev))) {
    printf("Smaky disk %s\n", ginfo->name);
    /* Show the list of a directory */
    if (!strcasecmp(choice, "list")) {
      if ((argc < 4 && harddisk == eFD) || (argc < 5 && harddisk == eHD)) {
        if (!list_dir(dev, "/"))
          res = -1;
      }
      else {
        file = strdup(argv[3]);
        if (!list_dir(dev, file))
          res = -1;
        free(file);
      }
    }
    /* Get a file from the disk */
    else if (argc >= 4 && !strcmp(choice, "get")) {
      if (argc >= 5 && strcmp(argv[4], "--harddisk"))
        dst = strdup(argv[4]);
      else
        dst = strdup("./");
      file = strdup(argv[3]);
      get_file(dev, argv[3], dst);
      free(file);
      free(dst);
    }
    else
      print_info();
  }
  /* Close the disk */
  fosfat_closedev(dev);
  free(path);
  free(choice);
  free(ginfo);
  return res;
}
