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

/** Disk types */
typedef enum disk_type {
    eFD,                        //!< Floppy Disk
    eHD                         //!< Hard Disk
} FOSFAT_DISK;

/** Data Block (256 bytes) */
typedef struct block_data {
    unsigned char data[256];     //!< Data
    /* Linked list */
    struct block_data *next_data;
} FOSFAT_DATA;

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
} FOSFAT_B0;

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
} FOSFAT_BLF;

/** Block List (256 bytes) */
typedef struct block_list {
    FOSFAT_BLF file[4];          //!< 4 BL files (240 bytes)
    unsigned char next[4];       //!< Next BL
    unsigned char chk[4];        //!< Check control
    unsigned char prev[4];       //!< Previous BL
    unsigned char reserve[4];    //!< Unused
    /* Linked list */
    struct block_list *next_bl;
} FOSFAT_BL;

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
} FOSFAT_BD;


unsigned long int c2l(unsigned char *value, int size);

/* Read the block 0 */
FOSFAT_B0 *fosfat_read_b0(FOSFAT_DEV *dev, unsigned long int block);

/* Read a folder and a file (LINKED LIST) */
FOSFAT_BD *fosfat_read_dir(FOSFAT_DEV *dev, unsigned long int block);
FOSFAT_BD *fosfat_read_file(FOSFAT_DEV *dev, unsigned long int block);
void fosfat_free_dir(FOSFAT_BD *var);
void fosfat_free_file(FOSFAT_BD *var);

/* Test attributes and type on a file */
int fosfat_isdir(FOSFAT_BLF *file);
int fosfat_isvisible(FOSFAT_BLF *file);
int fosfat_isopenexm(FOSFAT_BLF *file);
int fosfat_isencoded(FOSFAT_BLF *file);
int fosfat_issystem(FOSFAT_BLF *file);

/* Get a file */
int fosfat_get_file(FOSFAT_DEV *dev, FOSFAT_BD *file, const char *dst, int output);

/* Search */
FOSFAT_BD *fosfat_search_bd(FOSFAT_DEV *dev, const char *location, FOSFAT_BL *files);
FOSFAT_BD *fosfat_search_bd_insys(FOSFAT_DEV *dev, const char *location);

/* Open and close the device */
FOSFAT_DEV *fosfat_opendev(const char *dev, FOSFAT_DISK disk);
void fosfat_closedev(FOSFAT_DEV *dev);

#endif /* _FOSFAT_H_ */
