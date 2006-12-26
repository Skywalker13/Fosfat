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

/** Search type */
typedef enum search_type {
    eSBD,                        //!< Search BD
    eSBLF                        //!< Search BL File
} e_fosfat_search;

/** Data Block (256 bytes) */
typedef struct block_data {
    unsigned char data[256];     //!< Data
    /* Linked list */
    struct block_data *next_data;
} s_fosfat_data;

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

/** File in a Block List (60 bytes) */
typedef struct block_listf {
    char name[16];               //!< Filename
    unsigned char typ;           //!< Filetype
    unsigned char ope;           //!< Open counter
    unsigned char att[4];        //!< Attributes
    unsigned char lg[4];         //!< Length in blocks
    unsigned char lgb[2];        //!< Number of bytes in the last block
    unsigned char cd[3];         //!< Date of Creation
    unsigned char ch[3];         //!< Hour of Creation
    unsigned char wd[3];         //!< Date of the last Write
    unsigned char wh[3];         //!< Hour of the last Write
    unsigned char rd[3];         //!< Date of the last Read
    unsigned char rh[3];         //!< Hour of the last Read
    unsigned char secid[3];      //!< Security ID
    unsigned char clope;         //!< Open mode before CLOSE
    unsigned char pt[4];         //!< Pointer on the BD
    unsigned char lgf[4];        //!< File size in bytes
    unsigned char code[2];       //!< Code control
} s_fosfat_blf;

/** Block List (256 bytes) */
typedef struct block_list {
    s_fosfat_blf file[4];        //!< 4 BL files (240 bytes)
    unsigned char next[4];       //!< Next BL
    unsigned char chk[4];        //!< Check control
    unsigned char prev[4];       //!< Previous BL
    unsigned char reserve[4];    //!< Unused
    /* Linked list */
    struct block_list *next_bl;
} s_fosfat_bl;

/** Block Description (256 bytes) */
typedef struct block_desc {
    unsigned char next[4];       //!< Next BD
    unsigned char prev[4];       //!< Previous BD
    unsigned char npt[2];        //!< Number of tranches in the BD
    unsigned char pts[42][4];    //!< Pointers on the tranches (max 42 tranches)
    char name[16];               //!< Filename
    unsigned char nbs[42];       //!< Length (in blocks) of each tranches (1 byte)
    unsigned char reserve[4];    //!< Unused
    unsigned char lst[2];        //!< Number of byte in the last tranche
    unsigned char hac[2];        //!< Hashing function if LIST
    unsigned char nbl[2];        //!< Number of BL used if LIST
    unsigned char chk[4];        //!< Check control
    unsigned char off[4];        //!< Offset (in blocks) of all previous BD
    unsigned char free[2];       //!< Unused
    /* Linked list */
    struct block_desc *next_bd;
    struct block_list *first_bl;
} s_fosfat_bd;

/** Time */
typedef struct time {
    short int year;
    short int month;
    short int day;
    short int hour;
    short int minute;
    short int second;
} s_fosfat_time;

/** List of files in a directory */
typedef struct list_dir {
    char name[16];
    int size;
    s_fosfat_time time_c;
    s_fosfat_time time_w;
    s_fosfat_time time_r;
    /* Linked list */
    struct list_dir *next_file;
} s_fosfat_listdir;


unsigned long int c2l(unsigned char *value, int size);

/* Read the block 0 */
s_fosfat_b0 *fosfat_read_b0(FOSFAT_DEV *dev, unsigned long int block);

/* Read a folder and a file (LINKED LIST) */
s_fosfat_bd *fosfat_read_dir(FOSFAT_DEV *dev, unsigned long int block);
s_fosfat_bd *fosfat_read_file(FOSFAT_DEV *dev, unsigned long int block);
s_fosfat_listdir *fosfat_list_dir(FOSFAT_DEV *dev, const char *location);
void fosfat_free_dir(s_fosfat_bd *var);
void fosfat_free_file(s_fosfat_bd *var);
void fosfat_free_listdir(s_fosfat_listdir *var);

/* Test attributes and type on a file */
int fosfat_isdir(s_fosfat_blf *file);
int fosfat_isvisible(s_fosfat_blf *file);
int fosfat_isopenexm(s_fosfat_blf *file);
int fosfat_isencoded(s_fosfat_blf *file);
int fosfat_issystem(s_fosfat_blf *file);

/* Test attributes and type on a file since a location */
int fosfat_p_isdir(FOSFAT_DEV *dev, const char *location);
int fosfat_p_isvisible(FOSFAT_DEV *dev, const char *location);

/* Get a file */
int fosfat_get_file(FOSFAT_DEV *dev, s_fosfat_bd *file, const char *dst, int output);

/* Search */
void *fosfat_search_bdlf(FOSFAT_DEV *dev, const char *location, s_fosfat_bl *files, e_fosfat_search type);
void *fosfat_search_insys(FOSFAT_DEV *dev, const char *location, e_fosfat_search type);

/* Test */
int fosfat_isbdsys(FOSFAT_DEV *dev, s_fosfat_bd *sys);

/* Open and close the device */
FOSFAT_DEV *fosfat_opendev(const char *dev, e_fosfat_disk disk);
void fosfat_closedev(FOSFAT_DEV *dev);

#endif /* _FOSFAT_H_ */
