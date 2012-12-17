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
 *	 Routines to handle directories, file types and such.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ff_banked.h"
#include "dir_banked.h"

#include "types.h"
#include "dir.h"
#include "ff.h"
#include "mapper.h"
#include "mem_tools.h"
#include "data_tools.h"
#include "combsort.h"
/*
#include "keyb.h"
#include "screen.h"
#include "user_io.h"
*/

#pragma codeseg ("FILESYS")

const char exts[] = "D64T64PRGHEXMCSD71D81G64TAPSID";
const BYTE types[]    = { TYPE_D64, TYPE_T64, TYPE_PRG, TYPE_HEX, TYPE_HEX, TYPE_D71, TYPE_D81, TYPE_GCR, TYPE_TAP, TYPE_SID }; 
const BYTE extsout[] = "   DIRD64T64PRGPRGPRGHEXD71D81PRGPRGGCRTAPSIDTUN";

#define NUMEXT_IN  NR_OF_EL(types)


static char dir_compare(int a, int b);
static void dir_swap(int a, int b);

static void dir_sort(WORD dir_addr, WORD num_entries)
{
    MAPPER_MAP1 = dir_addr;

    combsort11(&dir_compare, &dir_swap, num_entries);
}

BYTE dir_change(DWORD cluster, DIRECTORY* directory, BYTE hide_dotnames)
{
	static FILINFO finfo;
	static FRESULT fres;
	static DIR     dir;
	
    static char ext[4];
	WORD entries = 0;
    BYTE b;
    
    // in this simple implementation, the directory will be read into memory
    DIRENTRY *entry = (DIRENTRY *)MAP1_ADDR;
    
	fres = f_opendir_direct(&dir, cluster);

    directory->num_entries = 0;
	directory->cbm_medium  = 0;
	
    if(fres != FR_OK) {
        return fres;	
    }
	
    MAPPER_MAP1 = directory->dir_address;
    ext[3] = 0;
	do {
        memset(entry, 0, sizeof(DIRENTRY));
		fres = f_readdir_lfn(&dir, &finfo, entry->fname, MAX_ENTRY_NAME, ext);
		if(fres == FR_OK) {
            if(finfo.fattrib & AM_HID)
                continue;
		    if(finfo.fattrib & AM_DIR) {
		        entry->ftype = TYPE_DIR;
		    } else {
		        entry->ftype = TYPE_UNKNOWN;
		        for(b=0;b<NUMEXT_IN;b++) {
		            if(strncmp(&exts[b*3], ext, 3)==0) {
		                entry->ftype = types[b];
		                break;
		            }
		        }
		    }
            if((entry->fname[0] == '.')&&(hide_dotnames))
                continue;
			if(finfo.fname[0]) {
		//            printf("%-13s %-29s %08lX %08lX '%s' %02X\n", finfo.fname, entry->fname, finfo.fsize, finfo.fcluster, ext, entry->ftype);
		        entry->fcluster = finfo.fcluster;
		        entry->fsize    = finfo.fsize;
		        entry->fattrib  = finfo.fattrib;
		        if(!(entry->fname[0])) // Already done in ff.c???
		           strncpy(entry->fname, &finfo.fname, MAX_ENTRY_NAME);

		        entry ++;
		        entries ++;
		    }
		}
	} while((fres == FR_OK) && (entry < (DIRENTRY *)MAP2_ADDR));	
	directory->num_entries = entries;
    directory->info_fields = 0; // all entries are selectable.
    
    dir_sort(directory->dir_address, entries);
    
    
	return FR_OK;
}

DIRENTRY *dir_getentry(WORD index, DIRECTORY *directory)
{
    static DIRENTRY *entry = (DIRENTRY *)MAP1_ADDR;
    MAPPER_MAP1 = directory->dir_address;

    if(index >= directory->num_entries)
        return NULL;
    
    return &(entry[index]);
}

WORD dir_getindex(DIRECTORY *directory, ENTRYNAME *entryname)
{
	static WORD index;
	static BYTE b, c1, c2;
	static BYTE name_len1;
	static BYTE name_len2;
	static BOOL match;
	
    static DIRENTRY *entry = (DIRENTRY *)MAP1_ADDR;
	
    MAPPER_MAP1 = directory->dir_address;
	name_len2 = data_strnlen(entryname->name_str, MAX_ENTRY_NAME);
	for(index = 0; index < directory->num_entries; index++) {
		
		name_len1 = data_strnlen(entry[index].fname, MAX_ENTRY_NAME);
		match = (name_len1 == name_len2);
		for(b=0; (b<name_len2) && (match); b++) {
			c1 = entryname->name_str[b];
			c2 = entry[index].fname[b];
			// case insensative see todo below
			if((c2 >= 0x61)&&(c2 <= 0x7a)) c2 -= 0x20; // TOBE: SMARTER
			if((c1 >= 0x61)&&(c1 <= 0x7a)) c1 -= 0x20; // TOBE: SMARTER
			if(c1 != c2) match = FALSE; // this is not a match
		}
		if(match) {	
			return index;
		}
	}
		// WE KNOW WHY--> TODO FIX IT!
#if 0 //stricmp should work but it doesnt
		
		b = stricmp(entry[index].fname, fname);
printf("comp <%s %s %d>\n\r", entry[index].fname, fname, b);
//		if(b == 0) {
//printf("match\n\r");
//			return index;
//		}
	}
#endif

	return 0xFFFF;

}

static
char dir_compare(int a, int b)
{
    static DIRENTRY *entry = (DIRENTRY *)MAP1_ADDR;

//    printf("compare %s and %s. (%d %dx)\n", &entry[a].fname, &entry[b].fname, a, b);
    if((entry[b].ftype == TYPE_DIR) && (entry[a].ftype != TYPE_DIR))
        return 1;
    if((entry[a].ftype == TYPE_DIR) && (entry[b].ftype != TYPE_DIR))
        return -1;

    return (char)strncmp(&entry[a].fname, &entry[b].fname, MAX_ENTRY_NAME);
}

static
void dir_swap(int a, int b)
{
    static DIRENTRY *entry = (DIRENTRY *)MAP1_ADDR;
//    static BYTE k;
//    printf("swap %d and %d.\n", a, b);

    memswap(&entry[a], &entry[b], sizeof(DIRENTRY));   
/*
    printf("Debug %04x %04x %02x.\n", debug_loc1, debug_loc2, k);
    for(k=0;k<24;k++) {
        printf("%02X ", stackframe[k]);
    } printf("\n");
*/
}    

/*
-------------------------------------------------------------------------------
							dir_find_entry
							==============
  Abstract:

	does patternmarching and stuff to find the best match entry in FF

-------------------------------------------------------------------------------
*/
DIRENTRY* dir_find_entry(ENTRYNAME *entryname, BYTE* types, DIRECTORY *directory)
{
	DIRENTRY *entry;
	WORD	dir_index;
	BYTE	typeindex;
	BOOL	match;

	match = FALSE;
	dir_index = directory->cbm_medium ? 1 : 0;
	while(!match) {
	    entry = dir_getentry(dir_index, directory);   
		dir_index ++;
		if((entry) && (entry->fname[0]) && (dir_index)) { // there is a name
			match = dir_match_name(entry, entryname, directory->cbm_medium);
			if(match && (types != NULL)) {
				for(match = FALSE, typeindex = 0; types[typeindex] != TYPE_TERMINATOR; typeindex++) {
					if(entry->ftype == types[typeindex]) {
						match = TRUE;
						break;
					}
				}
			}
		}
		else { 		
			entry = NULL;
			match = TRUE; // this is the best we can do
		}		
	}

	return entry;
}


/*
-------------------------------------------------------------------------------
							dir_match_name
							==========
  Abstract:

	sees if the name can be matched to the directory entry

  Parameters
	entry:		directory entry pointer
	
-------------------------------------------------------------------------------
*/
BOOL dir_match_name(DIRENTRY *entry, ENTRYNAME *entryname, BOOL casesensative)
{
	BOOL match = TRUE;
    BYTE b, c1, c2;
	
	if(!entryname->wildcard) { 
		// when no wildcard is issued while we are pattern matching the length must be correct
		if(entryname->len != data_strnlen(entry->fname, MAX_ENTRY_NAME)) {
			// no match
			match = FALSE;
		}
	}
	
	if(match) {
		for(b=0; (b<entryname->len) && match; b++) {
			c1 = entryname->name_str[b];
			c2 = entry->fname[b];
			if(!casesensative) {
				if((c2 >= 0x61)&&(c2 <= 0x7a)) c2 -= 0x20; // TOBE: SMARTER
				if((c1 >= 0x61)&&(c1 <= 0x7a)) c1 -= 0x20; // TOBE: SMARTER			
			}
			if(c1 == '?' && c2 != 0) continue; // always good when there is something
			if(c1 != c2) match = FALSE; // this is not a match
		}
	}
	
	return match;
}

/*
-------------------------------------------------------------------------------
							dir_analyze_name
							================
  Abstract:

	analyzes the name and sets the global variables.

  Parameters
	name:		Pointer to buffer containt zero terminated name
	
-------------------------------------------------------------------------------
*/
void dir_analyze_name(CHAR *name, BYTE maxlen, ENTRYNAME *entryname)
{
    static BYTE b, c, s, start, end;
	static BOOL commandpart;

	start = 0;

	commandpart = FALSE;
    entryname->len = data_strnlen(name, maxlen);
	entryname->unit = 0;
	entryname->name_str = NULL;
	entryname->command_str = NULL;
	entryname->command_len = 0;
	entryname->pattern = FALSE;	
	entryname->wildcard = FALSE;
	entryname->illegal = FALSE;
	entryname->replace = FALSE;
    entryname->param = NULL;
    
	start = 0;
    // checking for @ first allows @ without :
	if(name[0] == '@') {
		entryname->replace = TRUE;
        start = 1;
    }

    end = entryname->len;
    
    // preprocess: Cut up string in pieces
    for(b=start,s=0;b<entryname->len;b++) {
        c = name[b];
        switch(c) {
            case ':':
                entryname->pattern = FALSE;
                entryname->wildcard = FALSE;
                if(s==0) {
                    entryname->command_str = &name[start];
                    s=1;
                    name[b] = '\0';
                    start = b+1;
                } else {
                    entryname->illegal = TRUE; // no two :'s
                }
                break;
            case ',':
                entryname->param = &name[b+1];
                name[b] = 0;
                end = b;
                b = entryname->len; // end
                break;
            case '?':
                entryname->pattern = TRUE;
                break;
            case '*':
                entryname->wildcard = TRUE;
                end = b;
                break;
            case '/':
            case '\\':
            case '\"':
            case '|':
            case '<':
            case '>':
                entryname->illegal = TRUE;
                b = entryname->len; // end
                end = b;
                break;                
        }
    }
    entryname->name_str = &name[start];
    entryname->len = end - start;
    if(entryname->command_str)
        entryname->command_len = strlen(entryname->command_str);

    if(!entryname->command_len)
        entryname->command_str = NULL; // no characters left

    // now process command somewhat to see if we can find the unit number (and strip it)
    if(entryname->command_str) {
        c = entryname->command_str[entryname->command_len-1];
        if((c >= '0') && (c <= '3')) { // putting '9' here conflicts with MD64 command :-S
            entryname->unit = c - '0';
            entryname->command_len --;
            entryname->command_str[entryname->command_len] = '\0';
        }
        if(!entryname->command_len)
            entryname->command_str = NULL; // no characters left
    }
}


void dump_entryname(ENTRYNAME * entryname)
{
	printf("\nENTRYNAME -->\n");
	printf("BOOL replace %d\n", entryname->replace);	
	printf("BOOL pattern %d\n",	entryname->pattern);			
	printf("BOOL wildcard %d\n", entryname->wildcard);		
	printf("BOOL illegal %d\n", entryname->illegal);
	printf("BYTE len %d\n", entryname->len);
	printf("BYTE command_len %d\n", entryname->command_len);
	printf("BYTE unit %d\n", entryname->unit);
	if(entryname->command_str != NULL)
		printf("CHAR *command_str %s\n", entryname->command_str);
	if(entryname->name_str != NULL)
		printf("CHAR *name_str %s\n", entryname->name_str);
	if(entryname->param != NULL)
		printf("CHAR *param %s\n", entryname->param);
	printf("<-- ENTRYNAME\n");
}

