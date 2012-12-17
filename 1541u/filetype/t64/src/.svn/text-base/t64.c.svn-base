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
 *	 Functions that handle the T64 file format.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ff_banked.h"
#include "dir_banked.h"
#include "t64_banked.h"

#include "types.h"
#include "dir.h"
#include "ff.h"
#include "mapper.h"
#include "gcr.h"
#include "freezer.h"
#include "asciipetscii.h"

#pragma codeseg ("FILESYS")

static BYTE finfo[33];
static WORD bytes_read;

// file is already open, and can be referred to through &t64_file
FRESULT t64_dir(FIL *t64_file, DIRECTORY *directory, BOOL ascii_only)
{
    static BYTE b, v, s, c;
    static WORD strt, stop;
	static WORD entries;

    static WORD max;
    static WORD used;

	static CHAR *p;

    static DIRENTRY *entry;

	FRESULT fres;
    
    MAPPER_MAP1 = directory->dir_address;

    // clear directory
    entries = 0;
    entry = (DIRENTRY *)MAP1_ADDR;
    
    // read where to start reading the directory
    fres = f_lseek(t64_file, 0L);
	if(fres != FR_OK) return fres;

    fres = f_read(t64_file, finfo, 32, &bytes_read);
	if(fres != FR_OK) return fres;

    if((bytes_read != 32) || (finfo[0] != 0x43) || (finfo[1] != 0x36) || (finfo[2] != 0x34)) {
        printf("Not a valid T64 file. %d %s\n", bytes_read, finfo);
        return FALSE;
    }

    // read name of directory as first entry
    fres = f_read(t64_file, finfo, 32, &bytes_read);
	if(fres != FR_OK) return fres;	

	finfo[32] = 0;
	if(ascii_only) petscii2ascii((CHAR*)finfo, (CHAR*)finfo); // actually this is a form of formatting but I let it go

    for(b=0;b<MAX_ENTRY_NAME;b++) {
		if(ascii_only) {
	        entry->fname[b] = (finfo[b+8] & ~0x20) | 0x80; // actually this is a form of formatting but I let it go
		}
		else {
	        entry->fname[b] = finfo[b+8];
		}
    }
        
    entry->fsize = 0L;
    entry->fcluster = 0L;
    entry->fattrib = 0;
    entry->ftype = 0;
    entry++;
    entries++;
        
    // go and read directory!
    max  = *(WORD *)&finfo[2];
    used = *(WORD *)&finfo[4];
    
    for(;used;used--) {
        fres = f_read(t64_file, finfo, 32, &bytes_read);
		if(fres != FR_OK) return fres;
    
        if(finfo[0]) {
            memset(entry->fname, 0, NR_OF_EL(entry->fname)); 
            for(v=0,s=0;s<16;s++) { // v=0,
                c = finfo[16+s];
                if ((ascii_only) && ((c & 0x80)||(c == 32))) { // this is a sufficient conversion
                    entry->fname[s] = 32;
                }
				else {
                    entry->fname[s] = c;
                    v++;
                }
            }
			
			// eui: no trailing spaces please
			for(s=0;s<16;s++) {
				p = &entry->fname[15-s]; 
                if((*p == ' ') || (*p == 0xA0) || (*p == 0xE0)) {
					*p = 0;
                }
				else {
					if(*p != 0)
						break;
				}
			}
			
            if (v || (!ascii_only)) {
                strt = *(WORD *)&finfo[2];
                stop = *(WORD *)&finfo[4];
                printf("%s: %04x-%04x\n", entry->fname, strt, stop);
                *(WORD *)&(entry->fname[20]) = strt;  // patch to store start address
                entry->fcluster = *(LONG *)&finfo[8]; // file offset :)
                entry->fattrib = 0;
                entry->ftype = TYPE_PRGT64;
                entry->fsize = (stop)?(stop - strt):(65536 - strt);
                entry++;
                entries++;
            }
        }
    } 

	directory->num_entries = entries;
	directory->cbm_medium  = 2;
	directory->info_fields = 1;
	return fres;
}

FRESULT t64_readblock(FIL *t64_file, BYTE *block, BYTE *len, DWORD start, DWORD *next)
{
    static WORD bytes_read;
	FRESULT fres;
	
    fres = f_lseek(t64_file, start);
	if(fres != FR_OK) return fres;
	
	fres = f_read(t64_file, (void *)block, 255, &bytes_read);
	*next = start + bytes_read;
	*len = bytes_read;

	return fres;
}

FRESULT t64_loadfile(FIL *t64_file, DWORD start, WORD c64_strt, WORD length, BOOL runflag)
{
    static WORD space, map_nr, map_space, addr, end_addr;
	FRESULT fres;
	
    printf("t64_loadfile: offset=%08lX, start=%04x, length=%04x\n", start, c64_strt, length);
    dma_load_init();

    fres = f_lseek(t64_file, start);
	if(fres != FR_OK) return fres;

    addr = c64_strt;
    end_addr = c64_strt + length;

    map_nr = (addr >> 13) | MAP_RAMDMA;
    addr &= 0x1FFF;
    map_space = 0x2000- addr;
    addr |= MAP1_ADDR;

    printf("Loading from $%04X to $%04X.\n", addr, end_addr);
    // map_space   = bytes left in map
    // addr   = address in sd-cpu map
    // map_nr   = map #, pointing to C64 memory
    // end_addr   = end address
    
    do {
        MAPPER_MAP1 = map_nr;

        space = (map_space < length)?map_space:length;
        map_space -= space;
        length -= space;

        fres = f_read(t64_file, (void *)addr, space, &bytes_read);
		if(fres != FR_OK) return fres;

        printf("Bytes read: %04x (req=%04x) Len = %04x\n", bytes_read, space, length);

        if(!map_space) {
            addr = MAP1_ADDR;
            map_space = 0x2000;
            map_nr ++;
        } else {
            addr += space;
        }
    } while(length);

	if(fres != FR_OK) {
		// What now init freeze?

	}
	else {
	    // send end address of load
    	MAPPER_MAP1 = MAP_RAMDMA;
	    *(WORD *)(MAP1_ADDR + 0x002D) = end_addr;
    
    	dma_load_exit(runflag?DMA_RUN:DMA_BASIC);
	}

	return fres;
}

FRESULT t64_loadfirst(FIL *t64_file, BOOL runflag)
{
    static WORD strt, stop;
    static LONG len, offset;

	FRESULT fres;
	
    fres = f_lseek(t64_file, 0L);
    f_read(t64_file, finfo, 32, &bytes_read);

    if((bytes_read != 32) || (finfo[0] != 0x43) || (finfo[1] != 0x36) || (finfo[2] != 0x34)) {
        printf("Not a valid T64 file. %d %space\n", bytes_read, finfo);
        return FR_RW_ERROR;
    }

    fres = f_lseek(t64_file, 64L);
	
    f_read(t64_file, finfo, 32, &bytes_read);
	if(fres != FR_OK) return fres;

    strt = *(WORD *)&finfo[2];
    stop = *(WORD *)&finfo[4];
    len = (stop)?(stop - strt):(65536 - strt);
    offset = *(LONG *)&finfo[8]; // file offset :)

    fres = t64_loadfile(t64_file, offset, strt, len, runflag);

    return fres;
}
