/*
 *   1541 Ultimate - The Storage Solution for your Commodore computer.
 *   Copyright (C) 2009  Gideon Zweijtzer - Gideon's Logic Architectures
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Abstract:
 *	 Exports of directory functions.
 *
 */
#ifndef DIR_H
#define DIR_H

#include "types.h"
#include "ff.h"

#define NUMEXT_OUT   16

#define TYPE_TERMINATOR 255 // To use as list terminator
#define TYPE_UNKNOWN 0
#define TYPE_DIR     1
#define TYPE_D64     2
#define TYPE_T64     3
#define TYPE_PRG     4
#define TYPE_PRGD64  5
#define TYPE_PRGT64  6
#define TYPE_HEX     7
#define TYPE_D71     8
#define TYPE_D81     9
#define TYPE_PRGD71 10
#define TYPE_PRGD81 11
#define TYPE_GCR    12
#define TYPE_TAP    13
#define TYPE_SID    14
#define TYPE_TUNE   15

typedef struct _DIRECTORY {
	WORD dir_address; 	/* Where can the entry directory be found in RAM (which map that is) */
	WORD num_entries;	/* How many other entries there are */
	BYTE cbm_medium;	/* Is the entry on CBM medium (D64 (=1)/T64 (=2) etc) */
	BYTE info_fields;   /* Number of unselectable information fields */
} DIRECTORY;	


/* Specific administation about the entry */
typedef struct _ENTRYNAME {
	BOOL replace;		/* The entry name indicated this entry may replace another */
	BOOL pattern;		/* The entry name is a pattern in stead of a name */
	BOOL wildcard;		/* The entry name is using a wild card */
	BOOL illegal;		/* The entry name contains illegal characters (ignore if you wish) */
	BYTE len;			/* Length of the entry name */
	BYTE command_len;	/* Length of the command */
	BYTE unit;			/* The unit the entry is directed to */
	CHAR *command_str;	/* Pointer to the command string */
	CHAR *name_str;		/* Pointer to the entry name */
	CHAR *param;        /* Pointer to the parameters, if any */
} ENTRYNAME;


/* API */
BYTE dir_change(DWORD cluster, DIRECTORY* directory, BYTE hide_dotnames);
DIRENTRY *dir_getentry(WORD index, DIRECTORY *directory);
WORD dir_getindex(DIRECTORY *directory, ENTRYNAME *entryname);
DIRENTRY* dir_find_entry(ENTRYNAME *entryname, BYTE* types, DIRECTORY *directory);
BOOL dir_match_name(DIRENTRY *entry, ENTRYNAME *entryname, BOOL casesensative);
void dir_analyze_name(CHAR *name, BYTE maxlen, ENTRYNAME *entryname);

void dump_entryname(ENTRYNAME *entryname);
#endif
