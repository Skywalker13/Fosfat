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
} fosfat_disk_t;

/** Time */
typedef struct time_s {
  short int year;
  short int month;
  short int day;
  short int hour;
  short int minute;
  short int second;
} fosfat_time_t;

/** Attributes for a file (or dir) */
typedef struct att_s {
  int isdir     : 1;
  int isvisible : 1;
  int isencoded : 1;
  int islink    : 1;
} fosfat_att_t;

/** List of files in a directory */
typedef struct file_info_s {
  char name[FOSFAT_NAMELGT];
  int size;
  fosfat_att_t att;
  fosfat_time_t time_c;
  fosfat_time_t time_w;
  fosfat_time_t time_r;
  /* Linked list */
  struct file_info_s *next_file;
} fosfat_file_t;

/** Cache list for name, BD and BL blocks */
typedef struct cache_list_s {
  char *name;
  uint32_t bl;                 //!< BL Address
  uint32_t bd;                 //!< BD Address
  unsigned char isdir;         //!< If is a directory
  unsigned char islink;        //!< If is a soft link
  /* Linked list */
  struct cache_list_s *sub;
  struct cache_list_s *next;
} cachelist_t;

/** Main fosfat structure */
typedef struct fosfat_s {
  FOSFAT_DEV *dev;             //!< physical device
  int fosboot;                 //!< FOSBOOT address
  uint32_t foschk;             //!< CHK
  unsigned int cache;          //!< use cache system (search)
  cachelist_t *cachelist;      //!< cache data
} fosfat_t;


/* Disk */
char *fosfat_diskname(fosfat_t *fosfat);

/* Read a folder */
fosfat_file_t *fosfat_list_dir(fosfat_t *fosfat, const char *location);
void fosfat_free_listdir(fosfat_file_t *var);

/* Test attributes and type on a file since a location */
int fosfat_isdir(fosfat_t *fosfat, const char *location);
int fosfat_isvisible(fosfat_t *fosfat, const char *location);
int fosfat_isencoded(fosfat_t *fosfat, const char *location);
int fosfat_isopenexm(fosfat_t *fosfat, const char *location);

/* Get informations */
fosfat_file_t *fosfat_get_stat(fosfat_t *fosfat, const char *location);

/* Get a symlink's target */
char *fosfat_symlink(fosfat_t *fosfat, const char *location);

/* Get a file */
int fosfat_get_file(fosfat_t *fosfat, const char *src,
                    const char *dst, int output);
char *fosfat_get_buffer(fosfat_t *fosfat, const char *path,
                        int offset, int size);

/* Open and close the device */
fosfat_t *fosfat_open(const char *fosfat, fosfat_disk_t disk);
void fosfat_close(fosfat_t *fosfat);

/* Internal logger */
void fosfat_logger(unsigned char state);

#endif /* FOSFAT_H_ */
