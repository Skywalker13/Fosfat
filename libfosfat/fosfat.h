/*
 * FOS libfosfat: API for Smaky file system
 * Copyright (C) 2006-2007 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
 *
 * Thanks to Pierre Arnaud <pierre.arnaud@opac.ch>
 *           for its help and the documentation
 *    And to Epsitec SA for the Smaky computers
 *
 * This file is part of Fosfat.
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef FOSFAT_H_
#define FOSFAT_H_

#include <inttypes.h>

#define FOSFAT_DEV      FILE
#define FOSFAT_NAMELGT  17

/** Disk types */
typedef enum disk_type {
  eFD,                         //!< Floppy Disk
  eHD,                         //!< Hard Disk
  eDAUTO,                      //!< Auto Detection
  eFAILS                       //!< Auto Detection fails
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
  int islink    : 1;
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

/** Cache list for name, BD and BL blocks */
typedef struct cache_list {
  char *name;
  uint32_t bl;                 //!< BL Address
  uint32_t bd;                 //!< BD Address
  unsigned char isdir;         //!< If is a directory
  unsigned char islink;        //!< If is a soft link
  /* Linked list */
  struct cache_list *sub;
  struct cache_list *next;
} s_cachelist;

/** Main fosfat structure */
typedef struct fosfat {
  FOSFAT_DEV *dev;                  //!< physical device
  int fosboot;                      //!< FOSBOOT address
  uint32_t foschk;                  //!< CHK
  unsigned int cache;               //!< use cache system (search)
  s_cachelist *cachelist;           //!< cache data
} s_fosfat;


/* Disk */
char *fosfat_diskname(s_fosfat *fosfat);

/* Read a folder */
s_fosfat_file *fosfat_list_dir(s_fosfat *fosfat, const char *location);
void fosfat_free_listdir(s_fosfat_file *var);

/* Test attributes and type on a file since a location */
int fosfat_isdir(s_fosfat *fosfat, const char *location);
int fosfat_isvisible(s_fosfat *fosfat, const char *location);
int fosfat_isencoded(s_fosfat *fosfat, const char *location);
int fosfat_isopenexm(s_fosfat *fosfat, const char *location);

/* Get informations */
s_fosfat_file *fosfat_get_stat(s_fosfat *fosfat, const char *location);

/* Get a symlink's target */
char *fosfat_symlink(s_fosfat *fosfat, const char *location);

/* Get a file */
int fosfat_get_file(s_fosfat *fosfat, const char *src,
                    const char *dst, int output);
char *fosfat_get_buffer(s_fosfat *fosfat, const char *path,
                        int offset, int size);

/* Open and close the device */
s_fosfat *fosfat_open(const char *fosfat, e_fosfat_disk disk);
void fosfat_close(s_fosfat *fosfat);

/* Internal logger */
void fosfat_logger(unsigned char state);

#endif /* FOSFAT_H_ */
