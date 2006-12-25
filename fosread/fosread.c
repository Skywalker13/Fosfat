/*
 * FOS fosread: tool in read-only for Smaky file system
 * Copyright (C) 2006 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
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
#include <ctype.h>  /* tolower */

#include "fosfat.h"

#define VERSION     "0.0.1"

#define YEAR2K      0x73

typedef struct ginfo {
    char name[FOSFAT_NAMELGT];
} s_global_info;

/** Change a string in lower case.
 * @param data the string
 * @return a pointer on this string
 */
char *lc(char *data) {
    int i;

    for (i = 0; data[i] != '\0'; i++)
        data[i] = tolower(data[i]);
    return data;
}

/** Get info from the block 0.
 * @param dev pointer on the device
 * @return info
 */
s_global_info *get_ginfo(FOSFAT_DEV *dev) {
    s_fosfat_b0 *block0;
    s_global_info *ginfo;

    if ((block0 = fosfat_read_b0(dev, FOSFAT_BLOCK0))) {
        ginfo = (s_global_info *)malloc(sizeof(s_global_info));
        strcpy(ginfo->name, block0->nlo);
        ginfo->name[FOSFAT_NAMELGT - 1] = '\0';
        lc(ginfo->name);
    }
    else {
        printf("ERROR: I can't read the block 0!\n");
        ginfo = NULL;
    }

    free(block0);
    return ginfo;
}

/** Print date and hour.
 * @param dh date
 */
void print_date(unsigned char *dh) {
    printf(" %2i%02X-%02X-%02X %02X:%02X", dh[2] < YEAR2K ? 20 : 19, dh[2], dh[1], dh[0], dh[3], dh[4]);
}

/** Print a file in the list.
 * @param file BLF of a file
 */
void print_file(s_fosfat_blf *file) {
    int i;
    char filename[FOSFAT_NAMELGT];

    if (fosfat_isdir(file))
        snprintf(filename, strrchr(file->name, '.') - file->name + 1, "%s", file->name);
    else
        strncpy(filename, file->name, FOSFAT_NAMELGT - 1);
    filename[FOSFAT_NAMELGT - 1] = '\0';
    lc(filename);
    printf("%c%c%c%c %10li", fosfat_isdir(file) ? 'd' : '-',
                           fosfat_isvisible(file) ? '-' : 'h',
                           fosfat_issystem(file) ? 's' : '-',
                           fosfat_isencoded(file) ? 'e' : '-',
                           c2l(file->lgf, sizeof(file->lgf)));
    for (i = 0; i < 3; i++)
        print_date(file->cd + i * 6);
    printf(" %s\n", filename);
}

/** List the content of a directory.
 * @param dev pointer on the device
 * @param path where in the tree
 * @return true if it is ok
 */
int list_dir(FOSFAT_DEV *dev, const char *path) {
    int i, empty = 1;
    s_fosfat_bd *dir;
    s_fosfat_bl *files;

    if ((dir = fosfat_search_insys(dev, path, eSBD))) {
        files = dir->first_bl;
        printf("path: ");
        if (fosfat_isbdsys(dev, dir))
            printf("/\n");
        else
            printf("%s\n", path);
        printf("\n           size creation         last change      last view        filename\n");
        printf("           ---- --------         -----------      ---------        --------\n");
        do {
            /* Check all file in the BL */
            for (i = 0; i < 4; i++) {
                if (fosfat_isopenexm(&files->file[i]) && strlen(files->file[i].name) > 0) {
                    print_file(&files->file[i]);
                    empty = 0;
                }
            }
        } while ((files = files->next_bl));
        if (!empty)
            printf("\nd:directory  h:hidden  s:system  e:encoded\n");
        fosfat_free_dir(dir);
    }
    else {
        printf("ERROR: I can't read the SYS_LIST!\n");
        return 0;
    }
    return 1;
}

/** Copy a file from the disk.
 * @param dev pointer on the device
 * @param ginfo informations from the block 0
 * @param path where in the tree
 * @param dst where in local
 * @return true if it is ok
 */
int get_file(FOSFAT_DEV *dev, const char *path, const char *dst) {
    int res = 0;
    char *new_file;
    s_fosfat_bd *syslist, *file;
    s_fosfat_bl *files;

    new_file = strdup(dst);

    if ((syslist = fosfat_read_dir(dev, FOSFAT_SYSLIST))) {
        files = syslist->first_bl;
        file = fosfat_search_bdlf(dev, path, files, eSBD);
        if (file && !file->first_bl) {
            if (!strcasecmp(new_file, "./"))
                new_file = strdup(file->name);
            lc(new_file);
            printf("File \"%s\" is copying ...\n", path);
            if (fosfat_get_file(dev, file, lc(new_file), 1))
                res = 1;
            if (res)
                printf("Okay..");
            printf("\n");
        }
        else
            printf("ERROR: I can't found the file!\n");
        /* Freed variables */
        fosfat_free_dir(syslist);
        if (file && !file->first_bl)
            fosfat_free_file(file);
        else if (file)
            fosfat_free_dir(file);
    }
    else
        printf("ERROR: I can't read the SYS_LIST!\n");
    free(new_file);
    return res;
}

/** Print help. */
void print_info(void) {
    printf("Usage: fosread DEVICE MODE [NODE] [PATH] [--harddisk]\n");
    printf("Tool for a read-only access on a Smaky disk. Version %s\n\n", VERSION);
    printf("DEVICE       for example, /dev/fd0\n");
    printf("MODE\n");
    printf(" list        list the content of a node\n");
    printf(" get         copy a file from the Smaky's disk in a local directory\n");
    printf("NODE         the tree with the file (or folder) for a 'get' or a 'list'\n");
    printf("             example: foo/bar/toto.text\n");
    printf("PATH         you can specify a path for save the file (with get mode)\n");
    printf("--harddisk   if you use an hard disk and not a floppy, use this option\n");
    printf("\nPlease, report bugs to <mathieu.schroeter@gamesover.ch>\n");
}

int main(int argc, char **argv) {
    int res = 0;
    char *path, *choice, *file, *dst;
    FOSFAT_DEV *dev;
    s_global_info *ginfo;

    if (argc < 3) {
        print_info();
        return -1;
    }

    path = strdup(argv[1]);
    choice = strdup(argv[2]);

    /* Open the floppy disk (or hard disk) */
    if (!(dev = fosfat_opendev(path, strcmp(argv[argc - 1], "--harddisk") ? eFD : eHD))) {
        printf("Could not open %s for reading!\n", path);
        return -1;
    }

    /* Get globals informations on the disk */
    if ((ginfo = get_ginfo(dev))) {
        printf("Smaky disk %s\n", ginfo->name);
        /* Show the list of a directory */
        if (!strcasecmp(choice, "list")) {
            if (argc < 4) {
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
            if (argc == 5)
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
