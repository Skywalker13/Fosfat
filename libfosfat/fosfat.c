/*
 * FOS libfosfat: API for Smaky file system
 * Copyright (C) 2006 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
 *
 * Thanks to Pierre Arnaud <pierre.arnaud@opac.ch>
 *           for his help and the documentation
 *    And to Epsitec SA for the Smaky computers
 *
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
#include <ctype.h>      /* tolower */
#include <string.h>     /* strcasecmp strncasecmp strdup strlen strtok memcmp memcpy */

#include "fosfat.h"

#define MAX_SPLIT       64

/** List of all block types */
typedef enum block_type {
    eB0,                //!< Block 0
    eBL,                //!< Block List
    eBD,                //!< Block Description
    eDATA               //!< Only DATA
} e_fosfat_type;

/** Global variable for the FOSBOOT address */
static int g_fosboot = 0x10;


/** Translate a block number to an address. This function
 *  depend if the media is an HARD DISK or an 3"1/2 DISK.
 *  The FOSBOOT is the double in number of blocks for an
 *  hard disk.
 *  The real address is the address given by the disk + the
 *  size of the FOSBOOT part.
 * @param block the block's number given by the disk
 * @return the address of this block in the physical disk
 */
static inline unsigned long int blk2add(unsigned long int block) {
    return ((block + g_fosboot) * FOSFAT_BLK);
}

/** Math long int power function. The math.h library can
 *  calculate a pow only with floating point numbers.
 *  lpow is used only for the c2l() function.
 * @param x base value
 * @param y power
 * @return the result of x^y
 */
static long int lpow(long int x, long int y) {
    long int i, res = x;

    for (i = 1; i < y; i++)
        res *= x;
    /* x^0 return always 1 */
    if (!y)
        res = 1;
    return res;
}

/** Convert char table to an integer. This convertion will
 *  swapped all bytes and returned a long int value.
 * @param value pointer on the char table
 * @param size size of the table (number of bytes)
 * @return the long integer value
 */
static unsigned long int c2l(unsigned char *value, int size) {
    int i, j;
    unsigned long int res = 0;

    for (i = size - 1, j = 0; i >= 0; i--, j++)
        res += value[j] * lpow(16, (long int)i * 2);
    return res;
}

/** Change a string in lower case.
 * @param data the string
 * @return a pointer on this string
 */
static char *lc(char *data) {
    int i;

    for (i = 0; data[i] != '\0'; i++)
        data[i] = tolower(data[i]);
    return data;
}

/** Convert an integer in base 10 with the value shown
 *  in base 16, in an integer with the value shown in base 10.
 * @param val the value
 * @return the new integer
 */
static int h2d(int val) {
    char *conv;
    int res;

    conv = (char *)malloc(sizeof(val));

    snprintf(conv, sizeof(conv), "%X", val);
    res = atoi(conv);
    free(conv);
    return res;
}

/** Free a DATA file variable. It will be used only when
 *  a data is loaded for copy a file on the PC, for freed
 *  each block after each write.
 * @param var pointer on the data block
 */
static void fosfat_free_data(s_fosfat_data *var) {
    s_fosfat_data *d, *free_d;

    d = var;
    while (d) {
        free_d = d;
        d = d->next_data;
        free(free_d);
    }
}

/** Free a BD file variable. It must be used only with
 *  a BD that is (or can be) use as the first member of
 *  a linked list.
 *  This function must be used after all fosfat_read_file()!
 * @param var pointer on the description block
 */
static void fosfat_free_file(s_fosfat_bd *var) {
    s_fosfat_bd *bd, *free_bd;

    bd = var;
    while (bd) {
        free_bd = bd;
        bd = bd->next_bd;
        free(free_bd);
    }
}

/** Free a BD dir variable. It must be used only with
 *  a BD that is (or can be) use as the first member of
 *  a linked list for a dir (with BL linked list into).
 *  This function must be used after all fosfat_read_dir()!
 * @param var pointer on the description block
 */
static void fosfat_free_dir(s_fosfat_bd *var) {
    s_fosfat_bl *bl, *free_bl;
    s_fosfat_bd *bd, *free_bd;

    bd = var;
    do {
        bl = bd->first_bl;
        /* Freed all BL */
        while (bl) {
            free_bl = bl;
            bl = bl->next_bl;
            free(free_bl);
        }
        /* And after, freed the BD */
        free_bd = bd;
        bd = bd->next_bd;
        free(free_bd);
    } while (bd);
}

/** Free a List Dir variable.
 *  This function must be used after all fosfat_list_dir()!
 * @param var pointer on the description block
 */
void fosfat_free_listdir(s_fosfat_listdir *var) {
    s_fosfat_listdir *ld, *free_ld;

    ld = var;
    while (ld) {
        free_ld = ld;
        ld = ld->next_file;
        free(free_ld);
    }
}

/** Test if the file is a directory. This function read the ATT
 *  and return the value of the 12th bit, mask 0x1000.
 * @param file pointer on the file (in the BL)
 * @return a boolean (true for success)
 */
static inline int fosfat_isdir(s_fosfat_blf *file) {
    return (((int)c2l(file->att, sizeof(file->att)) & 0x1000));
}

/** Test if the file is visible. This function read the ATT
 *  and return the value of the 13th bit, mask 0x2000.
 * @param file pointer on the file (in the BL)
 * @return a boolean (true for success)
 */
static inline int fosfat_isvisible(s_fosfat_blf *file) {
    return (((int)c2l(file->att, sizeof(file->att)) & 0x2000));
}

/** Test if the file is 'open exclusif' and 'multiple'. This function
 *  read the ATT and return the value of the 1st or 2th bit,
 *  mask 0x1 and 0x2.
 * @param file pointer on the file (in the BL)
 * @return a boolean (true for success)
 */
static inline int fosfat_isopenexm(s_fosfat_blf *file) {
    return (((int)c2l(file->att, sizeof(file->att)) & 0x1)) ||
           (((int)c2l(file->att, sizeof(file->att)) & 0x2));
}

/** Test if the file is encoded. This function read the ATT
 *  and return the value of the 17th bit, mask 0x20000.
 * @param file pointer on the file (in the BL)
 * @return a boolean (true for success)
 */
static inline int fosfat_isencoded(s_fosfat_blf *file) {
    return (((int)c2l(file->att, sizeof(file->att)) & 0x20000));
}

/** Test if the file is system (5 MSB bits). This function read the
 *  TYP and return the value of the 3-7th bits, mask 0xF8.
 * @param file pointer on the file (in the BL)
 * @return a boolean (true for success)
 */
static inline int fosfat_issystem(s_fosfat_blf *file) {
    return ((int)(file->typ & 0xF8));
}

/** Test if the file is not deleted.
 * @param file pointer on the file (in the BL)
 * @return a boolean (true for success)
 */
static inline int fosfat_isnotdel(s_fosfat_blf *file) {
    return (file && strlen(file->name) > 0) ? 1 : 0;
}

/** Read a block defined by a type. This function read a block on
 *  the disk and return the structure in function of the type chosen.
 *  Each type use 256 bytes, but the structures are always bigger.
 *  The order of the attributes in each structures is very important,
 *  because the informations from the device are just copied directly
 *  without parsing.
 * @param dev pointer on the device
 * @param block block position
 * @param type type of this block (eB0, eBL, eBD or eDATA)
 * @return a pointer on the new block or NULL if broken
 */
static void *fosfat_read_b(FOSFAT_DEV *dev, unsigned long int block, e_fosfat_type type) {
    /* Move the pointer on the block */
    if (!fseek(dev, blk2add(block), SEEK_SET)) {
        switch (type) {
            case eB0: {
                s_fosfat_b0 *blk;
                blk = (s_fosfat_b0 *)malloc(sizeof(s_fosfat_b0));
                if (fread((s_fosfat_b0 *)blk, (size_t)sizeof(unsigned char), (size_t)FOSFAT_BLK, dev) == (size_t)FOSFAT_BLK)
                    return (s_fosfat_b0 *)blk;
                else
                    free(blk);
                break;
            }
            case eBL: {
                s_fosfat_bl *blk;
                blk = (s_fosfat_bl *)malloc(sizeof(s_fosfat_bl));
                if (fread((s_fosfat_bl *)blk, (size_t)sizeof(unsigned char), (size_t)FOSFAT_BLK, dev) == (size_t)FOSFAT_BLK)
                    return (s_fosfat_bl *)blk;
                else
                    free(blk);
                break;
            }
            case eBD: {
                s_fosfat_bd *blk;
                blk = (s_fosfat_bd *)malloc(sizeof(s_fosfat_bd));
                if (fread((s_fosfat_bd *)blk, (size_t)sizeof(unsigned char), (size_t)FOSFAT_BLK, dev) == (size_t)FOSFAT_BLK)
                    return (s_fosfat_bd *)blk;
                else
                    free(blk);
                break;
            }
            case eDATA: {
                s_fosfat_data *blk;
                blk = (s_fosfat_data *)malloc(sizeof(s_fosfat_data));
                if (fread((s_fosfat_data *)blk, (size_t)sizeof(unsigned char), (size_t)FOSFAT_BLK, dev) == (size_t)FOSFAT_BLK)
                    return (s_fosfat_data *)blk;
                else
                    free(blk);
            }
        }
    }
    return NULL;
}

/** Read the first usefull block (0). This block contents some
 *  informations on the disk. But no information are critical
 *  for read the file list.
 * @param dev pointer on the device
 * @param block block position
 * @return the block0 or NULL if broken
 */
s_fosfat_b0 *fosfat_read_b0(FOSFAT_DEV *dev, unsigned long int block) {
    return (s_fosfat_b0 *)fosfat_read_b(dev, block, eB0);
}

/** Read data block. This block contents only a char table of
 *  256 bytes for the raw data.
 * @param dev pointer on the device
 * @param block block position
 * @return the data or NULL if broken
 */
static inline s_fosfat_data *fosfat_read_d(FOSFAT_DEV *dev, unsigned long int block) {
    return (s_fosfat_data *)fosfat_read_b(dev, block, eDATA);
}

/** Read a Description Block (BD).
 * @param dev pointer on the device
 * @param block block position
 * @return the BD or NULL if broken
 */
static inline s_fosfat_bd *fosfat_read_bd(FOSFAT_DEV *dev, unsigned long int block) {
    return (s_fosfat_bd *)fosfat_read_b(dev, block, eBD);
}

/** Read a Block List (BL).
 * @param dev pointer on the device
 * @param block block position
 * @return the BL or NULL if broken
 */
static inline s_fosfat_bl *fosfat_read_bl(FOSFAT_DEV *dev, unsigned long int block) {
    return (s_fosfat_bl *)fosfat_read_b(dev, block, eBL);
}

/** Read data of some blocks and create linked list if necessary.
 *  When a directory or a file is read, the content is showed
 *  as data. But for a directory, really it is a BL. This function
 *  create the linked list for a DATA block or a BL for a number
 *  of consecutive blocks.
 * @param dev pointer on the device
 * @param block the first block (start) for the linked list
 * @param nbs number of consecutive blocks
 * @param type type of this block (eB0, eBL, eBD or eDATA)
 * @return the first block of the linked list created
 */
static void *fosfat_read_data(FOSFAT_DEV *dev, unsigned long int block, unsigned char nbs, e_fosfat_type type) {
    switch (type) {
        case eBL: {
            unsigned char i;
            s_fosfat_bl *block_list, *first_bl;

            if ((first_bl = fosfat_read_bl(dev, block))) {
                block_list = first_bl;
                for (i = 1; i < nbs; i++) {
                    block_list->next_bl = fosfat_read_bl(dev, block + (unsigned long int)i);
                    block_list = block_list->next_bl;
                }
                block_list->next_bl = NULL;
                return (s_fosfat_bl *)first_bl;
            }
            break;
        }
        case eDATA: {
            unsigned char i;
            s_fosfat_data *block_data, *first_data;

            if ((first_data = fosfat_read_d(dev, block))) {
                block_data = first_data;
                for (i = 1; i < nbs; i++) {
                    block_data->next_data = fosfat_read_d(dev, block + (unsigned long int)i);
                    block_data = block_data->next_data;
                }
                block_data->next_data = NULL;
                return (s_fosfat_data *)first_data;
            }
            break;
        }
        /* Only for no compilation warning because
         * all types are not in the switch
         */
        default:
            break;
    }
    return NULL;
}

/** Read a file description. A linked list is created for found
 *  all BD. And each BD have a linked list for found all DATA.
 * @param dev pointer on the device
 * @param block the file BD position
 * @return the first BD of the linked list
 */
s_fosfat_bd *fosfat_read_file(FOSFAT_DEV *dev, unsigned long int block) {
    unsigned long int next;
    s_fosfat_bd *file_desc, *first_bd;

    if ((file_desc = fosfat_read_bd(dev, block))) {
        file_desc->next_bd = NULL;
        file_desc->first_bl = NULL;     // Useless in this case
        first_bd = file_desc;
        /* Go to the next BD if exists (create the linked list for BD) */
        while ((next = c2l(file_desc->next, sizeof(file_desc->next)))) {
            file_desc->next_bd = fosfat_read_bd(dev, next);
            file_desc = file_desc->next_bd;
        }
        /* End of the BD linked list */
        file_desc->next_bd = NULL;
        return first_bd;
    }
    return NULL;
}

/** Get a file and put this in a location on the PC. This function
 *  read all BD->DATA of a file BD, and write the data in a
 *  new file on your disk. An output variable can be used for
 *  that the current size is printed for each PTS.
 *  The properties like "Creation Date" are not saved in the new
 *  file. All Linux file system are the same attributes for them
 *  files. And for example, ext2/3 have no "Creation Date".
 * @param dev pointer on the device
 * @param file file description block
 * @param dst destination on your PC
 * @param output TRUE for print the size
 * @return a boolean (true for success)
 */
static int fosfat_get(FOSFAT_DEV *dev, s_fosfat_bd *file, const char *dst, int output) {
    unsigned long int i;
    int res = 1;
    size_t check_last;
    size_t size = 0;
    FILE *f_dst;
    s_fosfat_data *file_d, *first_d;

    /* Create or replace a file */
    if ((f_dst = fopen(dst, "w"))) {
        /* Loop for all BD */
        do {
            /* Loop for all pointers */
            for (i = 0; i < c2l(file->npt, sizeof(file->npt)); i++) {
                if ((file_d = fosfat_read_data(dev, c2l(file->pts[i], sizeof(file->pts[i])), file->nbs[i], eDATA))) {
                    first_d = file_d;
                    /* Loop for all data blocks */
                    do {
                        /* Write the block */
                        check_last = (i == c2l(file->npt, sizeof(file->npt)) - 1 && !file_d->next_data) ?
                                      (size_t)c2l(file->lst, sizeof(file->lst)) : (size_t)FOSFAT_BLK;
                        if (fwrite((unsigned char *)file_d->data, (size_t)sizeof(unsigned char), check_last, f_dst) != check_last)
                            res = 0;
                        size += check_last;
                    } while (res && file_d->next_data && (file_d = file_d->next_data));
                    /* Freed all data */
                    fosfat_free_data(first_d);
                    if (res && output)
                        printf(" %i bytes\n", (int)size);
                }
                else
                    res = 0;
            }
        } while (res && file->next_bd && (file = file->next_bd));
        fclose(f_dst);
        /* If fails then remove the incomplete file */
        if (!res)
            remove(dst);
    }
    else
        res = 0;
    return res;
}

/** Read a complete .DIR (or SYS_LIST). A linked list is created
 *  for found all files. The first BD is returned. You can use
 *  this function for read all SYS_LIST, even in each folder. But
 *  the result will be always the same (recursive).
 *  Warning for not to do an infinite loop /!\
 * @param dev pointer on the device
 * @param block DIR (or SYS_LIST) BD position
 * @return the first BD of the linked list
 */
s_fosfat_bd *fosfat_read_dir(FOSFAT_DEV *dev, unsigned long int block) {
    unsigned long int i;
    unsigned long int next;
    s_fosfat_bd *dir_desc, *first_bd;
    s_fosfat_bl *dir_list;

    if ((dir_desc = fosfat_read_bd(dev, block))) {
        dir_desc->next_bd = NULL;
        dir_desc->first_bl = NULL;
        first_bd = dir_desc;
        do {
            /* Get the first pointer */
            dir_desc->first_bl = fosfat_read_data(dev, c2l(dir_desc->pts[0], sizeof(dir_desc->pts[0])), dir_desc->nbs[0], eBL);
            dir_list = dir_desc->first_bl;
            /* Go to the last BL */
            while (dir_list->next_bl)
                dir_list = dir_list->next_bl;

            /* Loop all others pointers */
            for (i = 1; i < c2l(dir_desc->npt, sizeof(dir_desc->npt)); i++) {
                dir_list->next_bl = fosfat_read_data(dev, c2l(dir_desc->pts[i], sizeof(dir_desc->pts[i])), dir_desc->nbs[i], eBL);
                dir_list = dir_list->next_bl;
                /* Go to the last BL */
                while (dir_list->next_bl)
                    dir_list = dir_list->next_bl;
            }
            /* End of the BL linked list */
            dir_list->next_bl = NULL;
        /* Go to the next BD if exists (create the linked list for BD) */
        } while ((next = c2l(dir_desc->next, sizeof(dir_desc->next))) &&
                 (dir_desc->next_bd = fosfat_read_bd(dev, next)) &&
                 (dir_desc = dir_desc->next_bd));
        /* End of the BD linked list */
        dir_desc->next_bd = NULL;
        return first_bd;
    }
    return NULL;
}

/** Search a BD or a BLF from a location. You can not use this
 *  function for found a BD of a system file /!\
 *  A good example for use this function, is the BL of the
 *  first SYS_LIST in the disk. It will search the file BD
 *  since this BL.
 *  The location must be not bigger of MAX_SPLIT /!\
 * @param dev pointer on the device
 * @param location path for found the BD (foo/bar/file)
 * @param files first BL for start the search
 * @param type eSBD or eSBLF
 * @return the BD, BLF or NULL is nothing found
 */
void *fosfat_search_bdlf(FOSFAT_DEV *dev, const char *location, s_fosfat_bl *files, e_fosfat_search type) {
    int i, j, nb = 0, ontop = 1;
    char *tmp, *path;
    char dir[MAX_SPLIT][FOSFAT_NAMELGT];
    s_fosfat_bl *loop;
    s_fosfat_bd *loop_bd = NULL;
    s_fosfat_blf *loop_blf = NULL;

    if (type)
        loop_blf = malloc(sizeof(s_fosfat_blf));

    loop = files;
    path = strdup(location);

    /* Split the path into a table */
    if ((tmp = strtok((char *)path, "/")) != NULL) {
        snprintf(dir[nb], sizeof(dir[nb]), "%s", tmp);
        while ((tmp = strtok(NULL, "/")) != NULL && nb < MAX_SPLIT - 1)
            snprintf(dir[++nb], sizeof(dir[nb]), "%s", tmp);
    }
    else
        snprintf(dir[nb], sizeof(dir[nb]), "%s", path);
    nb++;

    /* Loop for all directories in the path */
    for (i = 0; i < nb; i++) {
        ontop = 1;
        /* Loop for all BL */
        do {
            /* Loop for FOSFAT_NBL files in the BL */
            for (j = 0; j < FOSFAT_NBL; j++) {
                if (fosfat_isopenexm(&loop->file[j]) && !fosfat_issystem(&loop->file[j])) {
                    /* Test if it is a directory */
                    if (fosfat_isdir(&loop->file[j]) && !strncasecmp(loop->file[j].name, dir[i], strlen(loop->file[j].name) - 4)) {
                        if (type)
                            memcpy(loop_blf, &loop->file[j], sizeof(*loop_blf));
                        if (loop_bd)
                            fosfat_free_dir(loop_bd);
                        loop_bd = fosfat_read_dir(dev, c2l(loop->file[j].pt, sizeof(loop->file[j].pt)));
                        loop = loop_bd->first_bl;
                        ontop = 0;  // dir found
                        break;
                    }
                    /* Test if it is a file */
                    else if (!fosfat_isdir(&loop->file[j]) && !strcasecmp(loop->file[j].name, dir[i])) {
                        if (type)
                            memcpy(loop_blf, &loop->file[j], sizeof(*loop_blf));
                        if (loop_bd)
                            fosfat_free_dir(loop_bd);
                        loop_bd = fosfat_read_file(dev, c2l(loop->file[j].pt, sizeof(loop->file[j].pt)));
                        loop = NULL;
                        ontop = 0;  // file found
                        break;
                    }
                    else
                        ontop = 1;
                }
                else
                    ontop = 1;
            }
        } while (ontop && (loop = loop->next_bl));
    }
    free(path);
    if (!ontop) {
        if (type) {
            fosfat_free_dir(loop_bd);
            return (s_fosfat_blf *)loop_blf;
        }
        else
            return (s_fosfat_bd *)loop_bd;
    }
    return NULL;
}

/** Search a BD or a BLF from a location in the first SYS_LIST.
 *  It uses fosfat_search_bd().
 * @param dev pointer on the device
 * @param location path for found the BD (foo/bar/file)
 * @param type eSBD or eSBLF
 * @return the BD, BLF or NULL is nothing found
 */
void *fosfat_search_insys(FOSFAT_DEV *dev, const char *location, e_fosfat_search type) {
    s_fosfat_bd *syslist;
    s_fosfat_bl *files;
    void *search;

    if ((syslist = fosfat_read_dir(dev, FOSFAT_SYSLIST))) {
        files = syslist->first_bl;
        if ((search = fosfat_search_bdlf(dev, location, files, type))) {
            if (type)
                return (s_fosfat_blf *)search;
            else
                return (s_fosfat_bd *)search;
        }
        if (!type && (*location == '\0' || !strcmp(location, "/")))
            return (s_fosfat_bd *)syslist;
    }
    return NULL;
}

/** Test two blocks.
 * @param b1 block 1
 * @param b2 block 2
 * @return a boolean (true for success)
 */
static inline int fosfat_blkcmp(const void *b1, const void *b2) {
    return (memcmp(b1, b2, FOSFAT_BLK) == 0) ? 1 : 0;
}

/** Test if the BD is the first SYS_LIST.
 * @param dev pointer on the device
 * @param bd BD tested
 * @return a boolean (true for success)
 */
int fosfat_isbdsys(FOSFAT_DEV *dev, s_fosfat_bd *bd) {
    s_fosfat_bd *sys;

    sys = fosfat_read_dir(dev, FOSFAT_SYSLIST);
    return fosfat_blkcmp(bd, sys);
}

/** Test if the file is a directory.
 *  This function uses a string location.
 * @param dev pointer on the device
 * @param location file in the path
 * @return a boolean (true for success)
 */
int fosfat_p_isdir(FOSFAT_DEV *dev, const char *location) {
    s_fosfat_blf *entry;

    if (*location && strcmp(location, "/")) {
        if ((entry = fosfat_search_insys(dev, location, eSBLF)) &&
            fosfat_isnotdel(entry) && fosfat_isdir(entry)) {
            free(entry);
        }
        else
            return 0;
    }
    return 1;
}

/** Test if the file is visible.
 *  This function uses a string location.
 * @param dev pointer on the device
 * @param location file in the path
 * @return a boolean (true for success)
 */
int fosfat_p_isvisible(FOSFAT_DEV *dev, const char *location) {
    s_fosfat_blf *entry;

    if ((entry = fosfat_search_insys(dev, location, eSBLF)) &&
        fosfat_isnotdel(entry) && fosfat_isvisible(entry)) {
        free(entry);
    }
    else
        return 0;
    return 1;
}

/** Test if the file is encoded.
 *  This function uses a string location.
 * @param dev pointer on the device
 * @param location file in the path
 * @return a boolean (true for success)
 */
int fosfat_p_isencoded(FOSFAT_DEV *dev, const char *location) {
    s_fosfat_blf *entry;

    if ((entry = fosfat_search_insys(dev, location, eSBLF)) &&
        fosfat_isnotdel(entry) && fosfat_isencoded(entry)) {
        free(entry);
    }
    else
        return 0;
    return 1;
}

/** Return a linked list with all files of a directory.
 *  This function is high level.
 * @param dev pointer on the device
 * @param location directory in the path
 * @return the linked list
 */
s_fosfat_listdir *fosfat_list_dir(FOSFAT_DEV *dev, const char *location) {
    int i;
    s_fosfat_bd *dir;
    s_fosfat_bl *files;
    s_fosfat_listdir *listdir = NULL;
    s_fosfat_listdir *firstfile = NULL;

    if ((dir = fosfat_search_insys(dev, location, eSBD))) {
        /* Test if it is a directory */
        if (fosfat_p_isdir(dev, location)) {
            files = dir->first_bl;
            do {
                /* Check all files in the BL */
                for (i = 0; i < FOSFAT_NBL; i++) {
                    if (fosfat_isopenexm(&files->file[i]) && fosfat_isnotdel(&files->file[i]) && !fosfat_issystem(&files->file[i])) {
                        /* Complete the linked list with all files */
                        if (listdir) {
                            listdir->next_file = (s_fosfat_listdir *)malloc(sizeof(s_fosfat_listdir));
                            listdir = listdir->next_file;
                        }
                        else {
                            listdir = (s_fosfat_listdir *)malloc(sizeof(s_fosfat_listdir));
                            firstfile = listdir;
                        }
                        strncpy(listdir->name, files->file[i].name, sizeof(listdir->name));
                        lc(listdir->name);
                        listdir->size = (int)c2l(files->file[i].lgf, sizeof(files->file[i].lgf));

                        /* ATT field bits */
                        listdir->att.isdir = fosfat_isdir(&files->file[i]) ? 1 : 0;
                        listdir->att.isvisible = fosfat_isvisible(&files->file[i]) ? 1 : 0;
                        listdir->att.isencoded = fosfat_isencoded(&files->file[i]) ? 1 : 0;

                        /* Creation date */
                        listdir->time_c.year = h2d(files->file[i].cd[2]) + ((h2d(files->file[i].cd[2]) < FOSFAT_Y2K) ? 2000 : 1900);
                        listdir->time_c.month = h2d(files->file[i].cd[1]);
                        listdir->time_c.day = h2d(files->file[i].cd[0]);
                        listdir->time_c.hour = h2d(files->file[i].ch[0]);
                        listdir->time_c.minute = h2d(files->file[i].ch[1]);
                        listdir->time_c.second = h2d(files->file[i].ch[2]);
                        /* Writing date */
                        listdir->time_w.year = h2d(files->file[i].wd[2]) + ((h2d(files->file[i].wd[2]) < FOSFAT_Y2K) ? 2000 : 1900);
                        listdir->time_w.month = h2d(files->file[i].wd[1]);
                        listdir->time_w.day = h2d(files->file[i].wd[0]);
                        listdir->time_w.hour = h2d(files->file[i].wh[0]);
                        listdir->time_w.minute = h2d(files->file[i].wh[1]);
                        listdir->time_w.second = h2d(files->file[i].wh[2]);
                        /* Use date */
                        listdir->time_r.year = h2d(files->file[i].rd[2]) + ((h2d(files->file[i].rd[2]) < FOSFAT_Y2K) ? 2000 : 1900);
                        listdir->time_r.month = h2d(files->file[i].rd[1]);
                        listdir->time_r.day = h2d(files->file[i].rd[0]);
                        listdir->time_r.hour = h2d(files->file[i].rh[0]);
                        listdir->time_r.minute = h2d(files->file[i].rh[1]);
                        listdir->time_r.second = h2d(files->file[i].rh[2]);

                        listdir->next_file = NULL;
                    }
                }
            } while ((files = files->next_bl));
        }
        fosfat_free_dir(dir);
    }
    return firstfile;
}

/** Get a file and put this in a location on the PC.
 *  This function create a copy from src to dst.
 *  An output variable can be used for
 *  that the current size is printed for each PTS.
 * @param dev pointer on the device
 * @param src source on the Smaky disk
 * @param dst destination on your PC
 * @param output TRUE for print the size
 * @return a boolean (true for success)
 */
int fosfat_get_file(FOSFAT_DEV *dev, const char *src, const char *dst, int output) {
    int res = 0;
    s_fosfat_blf *file;
    s_fosfat_bd *file2;

    if ((file = fosfat_search_insys(dev, src, eSBLF)) &&
        fosfat_isnotdel(file) && !fosfat_isdir(file)) {
        file2 = fosfat_read_file(dev, c2l(file->pt, sizeof(file->pt)));
        if (fosfat_get(dev, file2, dst, output))
            res = 1;
        free(file);
        fosfat_free_file(file2);
    }
    return res;
}

/** Open the device. That hides the fopen processing.
 *  A device can be read like a file.
 * @param dev the device name
 * @param disk disk type
 * @return the device handle
 */
FOSFAT_DEV *fosfat_opendev(const char *dev, e_fosfat_disk disk) {
    switch (disk) {
        case eFD:
            g_fosboot = 0x10;
            break;
        case eHD:
            g_fosboot = 0x20;
    }
    return ((fopen(dev, "r")));
}

/** Close the device. That hides the fclose processing.
 * @param dev the device handle
 */
void fosfat_closedev(FOSFAT_DEV *dev) {
    fclose(dev);
}
