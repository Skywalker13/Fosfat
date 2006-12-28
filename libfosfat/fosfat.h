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

#ifndef _FOSFAT_H_
#define _FOSFAT_H_

#define FOSFAT_DEV      FILE
#define FOSFAT_NAMELGT  17

/** Disk types */
typedef enum disk_type {
    eFD,                         //!< Floppy Disk
    eHD                          //!< Hard Disk
} e_fosfat_disk;

/** Time */
typedef struct time {
    short int year;
    short int month;
    short int day;
    short int hour;
    short int minute;
    short int second;
} s_fosfat_time;

/** Attributes for a file (or dir) */
typedef struct att {
    int isdir     : 1;
    int isvisible : 1;
    int isencoded : 1;
} s_fosfat_att;

/** List of files in a directory */
typedef struct file_info {
    char name[FOSFAT_NAMELGT];
    int size;
    s_fosfat_att att;
    s_fosfat_time time_c;
    s_fosfat_time time_w;
    s_fosfat_time time_r;
    /* Linked list */
    struct file_info *next_file;
} s_fosfat_file;


/* Disk */
char *fosfat_diskname(FOSFAT_DEV *dev);

/* Read a folder */
s_fosfat_file *fosfat_list_dir(FOSFAT_DEV *dev, const char *location);
void fosfat_free_listdir(s_fosfat_file *var);

/* Test attributes and type on a file since a location */
int fosfat_p_isdir(FOSFAT_DEV *dev, const char *location);
int fosfat_p_isvisible(FOSFAT_DEV *dev, const char *location);
int fosfat_p_isencoded(FOSFAT_DEV *dev, const char *location);
int fosfat_p_isopenexm(FOSFAT_DEV *dev, const char *location);

/* Get informations */
s_fosfat_file *fosfat_get_stat(FOSFAT_DEV *dev, const char *location);

/* Get a file */
int fosfat_get_file(FOSFAT_DEV *dev, const char *src, const char *dst, int output);
char *fosfat_get_buffer(FOSFAT_DEV *dev, const char *path, int offset, int size);

/* Open and close the device */
FOSFAT_DEV *fosfat_opendev(const char *dev, e_fosfat_disk disk);
void fosfat_closedev(FOSFAT_DEV *dev);

#endif /* _FOSFAT_H_ */
