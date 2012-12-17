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
 *	 Functions that handle D64/D71 and D81 file formats.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Renames all functions here so they can reside in a seperate bank */
#include "ff_banked.h"
#include "dir_banked.h"
#include "d64_banked.h"

#include "types.h"
#include "dir.h"
#include "ff.h"
#include "mapper.h"
#include "gcr.h"
#include "freezer.h"
#include "asciipetscii.h"
#include "d64.h"
#include "dump_hex.h"

#pragma codeseg ("FILESYS")

extern FRESULT fres;

static DWORD ts2pos(BYTE t, BYTE s, BYTE m)
{
    return ((DWORD)ts2sectnr(s,t,m)) << 8;
}

static DWORD rootname_per_mode[] = { 91536L, 91536L, 399364L };
static BYTE  prgtype_per_mode[]  = { TYPE_PRGD64, TYPE_PRGD71, TYPE_PRGD81 };
static BYTE  dirtrack_per_mode[] = { 18, 18, 40 };
static BYTE  dirsect_per_mode[] =  {  1,  1,  3 };

// file is already open, and can be referred to through d64_file
FRESULT d64_dir(FIL *d64_file, DIRECTORY* directory, BOOL ascii_only, BYTE mode)
{
    static BYTE b, s, c, v;
    static BYTE cnt;
    static BYTE next_ts[2];
    static BYTE finfo[32];
    static WORD bytes_read;
    static WORD blocks;
	static WORD entries;
	static CHAR *p;
    static DIRENTRY *entry;
   	FRESULT fres;

    MAPPER_MAP1 = directory->dir_address;

	switch(mode) {
		case TYPE_D64:
			printf("mode D64");
			mode = 0;
			break;
		case TYPE_D71:	
			printf("mode D71");
			mode = 1;
			break;
		case TYPE_D81:
			printf("mode D81");			
			mode = 2;
			break;
	}

    // clear directory
    entries = 0;
    entry = (DIRENTRY *)MAP1_ADDR;
    
    // mockup for bad D64's that point to somewhere out of space
    next_ts[0] = dirtrack_per_mode[mode];
    next_ts[1] = dirsect_per_mode[mode];
   
    // read name of directory as first entry
    fres = f_lseek(d64_file, rootname_per_mode[mode]); // 18 00 offset $90 
	if(fres != FR_OK) return fres;

    fres = f_read(d64_file, finfo, MAX_ENTRY_NAME, &bytes_read);
	if(fres != FR_OK) return fres;
	finfo[MAX_ENTRY_NAME] = 0;
	
	if(ascii_only) petscii2ascii((CHAR*)finfo,(CHAR*)finfo); // actually this is a kind of formatting but I let it go
	
    for(b=0;b<MAX_ENTRY_NAME;b++) {
		if(ascii_only) {
	        entry->fname[b] = (finfo[b] & ~0x20) | 0x80; // actually this is a kind of formatting but I let it go
		}
		else {
			entry->fname[b] = finfo[b];
		}
    }
			
    entry->fsize = 0L;
    entry->fcluster = 0L;
    entry->fattrib = 0;
    entry->ftype = 0;
    entry++;
    entries++;
    cnt = 0;

    // go and read directory!
    do {
//        printf("Next ts: %02X %02X %08lX\n", next_ts[0], next_ts[1], ts2pos(next_ts[0], next_ts[1]));
        fres = f_lseek(d64_file, ts2pos(next_ts[0], next_ts[1], mode));
		if(fres != FR_OK) return fres;
            
        fres = f_read(d64_file, next_ts, 2, &bytes_read);
    	if(fres != FR_OK) return fres;

        for(b=0;b<8;b++) {
            fres = f_read(d64_file, finfo, 32, &bytes_read);
			if(fres != FR_OK) return fres;

//			dump_hex(finfo, 32);
			
            if(finfo[3]) {
                memset(entry->fname, 0, NR_OF_EL(entry->fname));
                for(v=0,s=0;s<16;s++) { // v=0,
                    c = finfo[3+s];
                    if ((ascii_only) && ((c & 0x80)||(c == 32))) { // this is a sufficient conversion
                        entry->fname[s] = 32;
                    } else {
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

//                if(finfo[0]) {
                if(!ascii_only || v) {
                    entry->fcluster = ts2pos(finfo[1], finfo[2], mode); // track/sector to start byte on disk :)
//                    printf("cluster=%08lX, because ts=%d,%d\n", entry->fcluster,finfo[1],finfo[2]);
                    entry->fattrib = 0;
    				if((finfo[0] & 0x0f)==0x02) {
                        entry->ftype = prgtype_per_mode[mode]; //finfo[0]; eui: for now ok but todo seq, rel, usr, del?
                    } else {
                        entry->ftype = TYPE_UNKNOWN;
                    }
                    blocks = (WORD)finfo[28] | (((WORD)finfo[29]) << 8);
                    entry->fsize = (DWORD)blocks; //(((DWORD)blocks) << 8) - (blocks << 1);  // * 254
                    entry++;
                    entries++;
                }
//                }
            } 
			else {
                break;
            }
        }
    } 
    while(next_ts[0] && (++cnt < 250));

	directory->num_entries = entries;
	directory->cbm_medium  = 1;
	directory->info_fields = 1;
	return fres;
}


FRESULT d64_readblock(FIL *d64_file, BYTE *block, BYTE *len, DWORD start, DWORD *next, BYTE mode)
{
    static BYTE next_ts[2];
    static WORD bytes_in_block, bytes_read;
	FRESULT fres;

	switch(mode) {
		case TYPE_PRGD64:
			printf("mode D64");			
			mode = 0;
			break;
		case TYPE_PRGD71:
			printf("mode D71");
			mode = 1;
			break;
		case TYPE_PRGD81:
			printf("mode D81");
			mode = 2;
			break;
	}
	
    fres = f_lseek(d64_file, start);
	if(fres != FR_OK) return fres;

    fres = f_read(d64_file, next_ts, 2, &bytes_read);
	if(fres != FR_OK) return fres;
	
    if(next_ts[0])
        bytes_in_block = 254;
    else
        bytes_in_block = (WORD)next_ts[1];

	*next = ts2pos(next_ts[0], next_ts[1], mode);

	fres = f_read(d64_file, (void *)block, bytes_in_block, &bytes_read);
	*len = bytes_read;

	return fres;
}

FRESULT d64_loadfile(FIL *d64_file, DWORD start, BOOL runflag, BYTE mode)
{
    static BYTE next_ts[2];
    static WORD space, map_nr, map_space, addr, end_addr, bytes_in_block, bytes_read;
    static LONG filepos;
    
	FRESULT fres;

	switch(mode) {
		case TYPE_PRGD64:
			printf("mode D64");
			mode = 0;
			break;
		case TYPE_PRGD71:
			printf("mode D71");
			mode = 1;
			break;
		case TYPE_PRGD81:
			printf("mode D81");
			mode = 2;
			break;
	}

    dma_load_init();

    printf("D64_Load file: Start = %08lx\n", start);
    if(start > d64_file->fsize) {
        printf("Start seems to be out of bounds. Exit loader.\n");
    	dma_load_exit(DMA_BASIC);
        return FR_OUT_OF_RANGE;
    }
        
    fres = f_lseek(d64_file, start);
	if(fres == FR_OK) {
	    fres = f_read(d64_file, next_ts, 2, &bytes_read);
	}

	if(fres == FR_OK) {
	    if(next_ts[0])
	        bytes_in_block = 254;
	    else
	        bytes_in_block = (WORD)next_ts[1];

	    fres = f_read(d64_file, (BYTE *)&addr, 2, &bytes_read);
	}

	if(fres == FR_OK) {
	    bytes_in_block -= 2;
		
//    printf("start = %08lX. next=%d,%d. addr = %04X. BR=%d.\n", start, next_ts[0],next_ts[1],addr,bytes_read);

	    map_nr = (addr >> 13) | MAP_RAMDMA;
	    end_addr = addr;
	    addr &= 0x1FFF;
	    map_space = 0x2000 - addr;
	    addr |= MAP1_ADDR;

	    // bytes_in_block = bytes left in block
	    // map_space   = bytes left in map
	    // addr   = address in sd-cpu map
	    // map_nr   = map #, pointing to C64 memory
	    // end_addr   = end address
	    
	    do {
	        MAPPER_MAP1 = map_nr;

	        if(bytes_in_block > map_space) { // block wraps over mapper boundary
	            space = map_space;
	        } else { // block still fits in mapper page
	            space = bytes_in_block;
	        }
	        fres = f_read(d64_file, (void *)addr, space, &bytes_read);
			if(fres != FR_OK) break;
			
	        bytes_in_block -= space;
	        map_space   -= space;
	        end_addr   += space;

//	        printf("%d bytes read to addr %04x. Requested %04x bytes. map_space=%04x. End_addr=%04x\n", bytes_read, addr, space, map_space, end_addr);

	        if(!map_space) {
	            addr = MAP1_ADDR;
	            map_space = 0x2000;
	            map_nr ++;
	        } else {
	            addr += space;
	        }
	        if((!bytes_in_block)&&(next_ts[0])) {
                filepos = ts2pos(next_ts[0], next_ts[1], mode);
//                printf("($%lx,%d,%d).\n", filepos, next_ts[0], next_ts[1]);
                if(filepos > d64_file->fsize) {
                    printf("Disk pointer seems to be out of bounds (%ld,%d,%d). Exit loader.\n", filepos, next_ts[0], next_ts[1]);
                    fres = FR_OUT_OF_RANGE;
                    break;
                }
	            fres = f_lseek(d64_file, filepos);
				if(fres != FR_OK) break;

	            fres = f_read(d64_file, next_ts, 2, &bytes_read);
				if(fres != FR_OK) break;

	            if(next_ts[0])
	                bytes_in_block = 254;
	            else
	                bytes_in_block = (WORD)next_ts[1];
	        }                            
	    } while(bytes_in_block);
	}

	if(fres != FR_OK) {
		// What now init freeze?
    	dma_load_exit(DMA_BASIC);
	}
	else {
	    // send end address of load
    	MAPPER_MAP1 = MAP_RAMDMA;
	    *(WORD *)(MAP1_ADDR + 0x002D) = end_addr - 1;
    
    	dma_load_exit(runflag?DMA_RUN:DMA_BASIC);
	}

	return fres;
}

// file should already be open, and can be referred to through &mountfile
FRESULT d64_write_track(FIL* mountfile, BYTE tr)
{
    static DWORD file_offs;
    static BYTE n,c;
    static WORD bytes_written;
    static BYTE *strt;
	FRESULT fres;

    MAPPER_MAP1H = 0;
    MAPPER_MAP1L = tr;

    file_offs = ts2pos(tr+1, 0, 0);
    n = get_num_sectors(tr, 0);
    
    if((!mountfile->org_clust)||(!mountfile->fsize)) {
        printf("ERROR: Can't write to closed or zero length file!\n");
        return FR_NO_FILE;
    } // TODO: je mag alleen schrijven als de FILE de juiste lengte heeft

    printf("Track: %02d, Start = %04X, Offset = %08lX. Num sectors = %d\n", tr+1, find_trackstart(), file_offs, n);

    for(c=0;c<n;c++) {
        strt = get_sector(tr);
//        printf("  Header (%04x): ", strt);
//        for(d=0;d<7;d++) {
//            printf("%02x ", strt[d]);
//        }
        file_offs = ts2pos(tr+1, header[2], 0);
//        printf("  Pos: %08lX\n", file_offs);
        f_lseek(mountfile, file_offs);
        fres = f_write(mountfile, &sector_buf[1], 256, &bytes_written);
        if(fres != FR_OK) {
            printf("Error writing. %d\n", fres);
			return fres;
        }
        if(bytes_written != 256) {
            printf("Not 256 bytes written, but %d\n", bytes_written);
			return fres;
        }
    }

	return fres;
}

extern BYTE bam_header[144];

FRESULT d64_create(FIL *f, char *name)
{
    // f is a pointer to a file that was just created in the menu
    // all tracks can contain crap, as long as track 18 is correct
    BYTE t,c,b;
//    WORD n;
    WORD bw;
    FRESULT fres;

    // first half of second block of track 18
    for(t=0;t<160;t++) {
        sector_buf[t] = 0;
    }
    sector_buf[1] = 0xFF;

    // part that comes after bam header
    for(t=0;t<27;t++) {
        sector_buf[128+t] = 0xA0;
    }
    sector_buf[149] = '2';
    sector_buf[150] = 'A';
    
    for(t=0,b=0;t<27;t++) {
        c = name[b++];
        if(!c)
            break;
        if((c >= 'a')&&(c <= 'z'))
            c -= 0x20; // capitalize
        if(c == ',') {
            if(name[b]) sector_buf[146] = name[b++];
            if(name[b]) sector_buf[147] = name[b];
            break;
        } else {
            sector_buf[128+t] = c;
        }
    }            
    
/*
    // write crap from sram address zero.
    MAPPER_MAP2 = 0;
    for(t=0;t<35;t++) {
        n = (WORD)get_num_sectors(t, 0);
        n <<= 8;
        fres = 0;
        if(t == 17) {
            fres = f_write(f, bam_header, 144, &bw);
            fres |= f_write(f, &sector_buf[128], 32, &bw);
            fres |= f_write(f, &sector_buf[16], 80, &bw);
            fres |= f_write(f, sector_buf, 128, &bw);
            sector_buf[1] = 0;
            fres |= f_write(f, sector_buf, 128, &bw);
            n -= 512;
        }
        fres |= f_write(f, (void *)MAP2_ADDR, n, &bw);
        printf("tr = %d. fres = %d. bw = %d\n", t+1, fres, bw);

        if(fres != FR_OK)
            return fres;
//        if(bw != n)
//            return FR_RW_ERROR;
    }            
*/
    fres = f_lseek(f, D64_BAMLOC);
    fres |= f_write(f, bam_header, 144, &bw);
    fres |= f_write(f, &sector_buf[128], 32, &bw);
    fres |= f_write(f, &sector_buf[16], 80, &bw);
    fres |= f_write(f, sector_buf, 128, &bw);
    sector_buf[1] = 0;
    fres |= f_write(f, sector_buf, 128, &bw);

    return FR_OK;
}

FRESULT d64_save(FIL *f, char *error)
{
    BYTE b,c,d;
    WORD w;
    BYTE *strt;
    FRESULT fres;

    MAPPER_MAP1H = 0;
    for(b=0;b<35;b++) {
        MAPPER_MAP1L = b;
        w = find_trackstart();
        if(!w) {
            sprintf(error, "Cannot find header for sector 0 on track %d. ", b+1);
            return FR_RW_ERROR;
        } else { // start found
            d = get_num_sectors(b, 0);
            for(c=0;c<d;c++) {
                strt = get_sector(b);
                if(header[2] != c) {
                    sprintf(error, "Wrong sector on track %d. Hdr=%02x Exp:%02x. ", b+1, header[2], c);
                    return FR_RW_ERROR;
                }
                if(header[3] != (b+1)) {
                    sprintf(error, "Wrong track # on T/S %d/%d. Hdr=d. ", b+1, c, header[3]);
                    return FR_RW_ERROR;
                }
				fres = f_write(f, &sector_buf[1], 256, &w);
                if(fres != FR_OK) return fres;
                printf(".");
            }
            printf("\n");
        }
    }
    return fres;
}

#define PARAM_BASE 0x2C00  // this should move to some header file

void d64_load_params(void)
{
    ULONG *param;
    ULONG loc, leng;
    BYTE  i,reg;

    BYTE  region_end[] = { 17, 24, 30, 255 };
//    ULONG region_len[] = { 7692, 7143, 6667, 6250 };
//    .word   $1E00, $1BE0, $1A00, $1860, $1E00, $1BE0, $1A00, $1860
    ULONG region_len[] = { 0x1DFF, 0x1BDF, 0x19FF, 0x185F };
    
    param = (ULONG *)PARAM_BASE;
    leng  = region_len[0];
    
    for(i=1,reg=0;i<=42;i++) {
        loc = ((ULONG)(i-1)) << 13;
        *(param++) = loc;
        *(param++) = leng;
        *(param++) = loc;
        *(param++) = leng;
//        *(param++) = 0; //loc;
//        *(param++) = 1; //leng;
//        printf("loc/len written: %06lx %06lx\n", loc,leng);
        if(i==region_end[reg]) {
            reg++;
            leng = region_len[reg];
        }
    }
}
