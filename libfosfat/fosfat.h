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

/* Block size (256 bytes) */
#define FOSFAT_BLK      256

#define FOSFAT_BLOCK0   0x00
#define FOSFAT_SYSLIST  0x01

#define FOSFAT_DEV      FILE
#define FOSFAT_NAMELGT  17
#define FOSFAT_NBL      4
#define FOSFAT_Y2K      70

/** Disk types */
typedef enum disk_type {
    eFD,                         //!< Floppy Disk
    eHD                          //!< Hard Disk
} e_fosfat_disk;

/** Block 0 (256 bytes) */
typedef struct block_0 {
    unsigned char sys[44];       //!< SYSTEM folder
    char nlo[16];                //!< Disk name
    unsigned char chk[4];        //!< Check control
    unsigned char mes[172];      //!< Message
    unsigned char change;        //!< Need of change the CHK number
    unsigned char bonchk;        //!< New format
    unsigned char oldchk[4];     //!< Old CHK value
    unsigned char newchk[4];     //!< New CHK value
    unsigned char reserve[10];   //!< Unused
} s_fosfat_b0;

/** Time */
typedef struct time {
    short int year;
    short int month;
    short int day;
    short int hour;
    short int minute;
    short int second;
} s_fosfat_time;

typedef struct att {
    int isdir     : 1;
    int isvisible : 1;
    int isencoded : 1;
} s_fosfat_att;

/** List of files in a directory */
typedef struct list_dir {
    char name[16];
    int size;
    s_fosfat_att att;
    s_fosfat_time time_c;
    s_fosfat_time time_w;
    s_fosfat_time time_r;
    /* Linked list */
    struct list_dir *next_file;
} s_fosfat_listdir;


/* Read the block 0 */
s_fosfat_b0 *fosfat_read_b0(FOSFAT_DEV *dev, unsigned long int block);

/* Read a folder */
s_fosfat_listdir *fosfat_list_dir(FOSFAT_DEV *dev, const char *location);
void fosfat_free_listdir(s_fosfat_listdir *var);

/* Test attributes and type on a file since a location */
int fosfat_p_isdir(FOSFAT_DEV *dev, const char *location);
int fosfat_p_isvisible(FOSFAT_DEV *dev, const char *location);
int fosfat_p_isencoded(FOSFAT_DEV *dev, const char *location);

/* Get a file */
int fosfat_get_file(FOSFAT_DEV *dev, const char *src, const char *dst, int output);

/* Open and close the device */
FOSFAT_DEV *fosfat_opendev(const char *dev, e_fosfat_disk disk);
void fosfat_closedev(FOSFAT_DEV *dev);

#endif /* _FOSFAT_H_ */
