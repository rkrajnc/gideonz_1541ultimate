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
 *	 Functions that handle the SID file format.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "dir.h"
#include "ff.h"
#include "mapper.h"
#include "freezer.h"
#include "gpio.h"
#include "cartridge.h"

#define SWAP(t,x,y) t=x; x=y; y=t;
extern BYTE current_drv;
extern BYTE sid_player[];
static BYTE sid_header[0x80];

const DWORD magic_psid = 0x44495350; // little endian assumed
const DWORD magic_rsid = 0x44495352; // little endian assumed
const BYTE  string_offsets[3] = { 0x16, 0x36, 0x56 };

// file is already open, and can be referred to through &sid_file
FRESULT sid_dir(FIL *sid_file, DIRECTORY *directory)
{
    static BYTE b,i;
	static WORD entries;
    static WORD bytes_read;
    static DWORD *magic;
    
    static DIRENTRY *entry;

	FRESULT fres;
    
    MAPPER_MAP1 = directory->dir_address;

    // clear directory
    entries = 0;
    entry = (DIRENTRY *)MAP1_ADDR;
    
    fres = f_read(sid_file, sid_header, 0x7e, &bytes_read);
	if(fres != FR_OK) return fres;

    // header checks
    magic = (DWORD *)sid_header;
    if((*magic != magic_rsid)&&(*magic != magic_psid)) {
        printf("Filetype not as expected. (%08lx)\n", *magic); 
        return FALSE;
    }

    for(b=0;b<3;b++) {
        memcpy(&entry->fname[0], &sid_header[string_offsets[b]], MAX_ENTRY_NAME);
        for(i=0;i<MAX_ENTRY_NAME;i++) {
            entry->fname[i] |= 0x80;
        }
        entry->fattrib = 0;
        entry->fsize = 0L;
        entry->fcluster = 0;
        entry->ftype = 0;
        entry++;
        entries++;
    }    

    // number of tunes is in 0x0F (assuming never more than 256 tunes)
    for(b=1;b<=sid_header[0x0f];b++) {
        sprintf(&entry->fname[0], "Tune #%d", b);
        entry->fattrib = 0;
        entry->fsize = 0L;
        entry->fcluster = (DWORD)b;
        entry->ftype = TYPE_TUNE;
        entry++;
        entries++;
    }

	directory->num_entries = entries;
	directory->cbm_medium  = 3;
	directory->info_fields = 3;
	return fres;
}

BOOL sid_load_init(void)
{
    static WORD timeout;
    static BYTE b;
    
    MAPPER_MAP1 = MAP_RAMDMA;

    GPIO_IMASK  = 0;    // turn off all interrupts
    CART_BASE   = 0x1C; // SID player base
    CART_MODE   = 0x01; // 8k rom, rom based

    // reset c64
    GPIO_SET2   = C64_RESET;
    
    TIMER     = 200;
    while(TIMER)
        ;

    GPIO_CLEAR2 = C64_RESET;
    
    timeout = 0;
    
    // init test bytes for handshake
    stop_c64(FALSE);
    *(BYTE *)(MAP1_ADDR + 0x02)  = 0x00;
    *(BYTE *)(MAP1_ADDR + 0x142) = 0x00;

    while((*(BYTE *)(MAP1_ADDR + 0x02)) == 0x00) {
        run_c64();
        timeout++;
        if(timeout == 200)
            return FALSE;
        for(b=0;b<20;b++)  {
            TIMER = 255;
            while(TIMER)
                ;
        }
        printf("/");
        stop_c64(FALSE);
    }

    enable_cartridge(); // enable selected cart again, will be activated upon reset

    return TRUE;
}

void sid_load_exit(void)
{
    // signal done to dma program in C64
    MAPPER_MAP1 = MAP_RAMDMA;
    *(BYTE *)(MAP1_ADDR + 0x0142) = 0xAA;

    // fix drive number
    *(BYTE *)(MAP1_ADDR + 0x00BA) = current_drv;
    // fix load status
    *(BYTE *)(MAP1_ADDR + 0x0090) = 0x40;
    // fix FRESPC pointer
    *(WORD *)(MAP1_ADDR + 0x0035) = 0xA000;
    
    run_c64();
}
    
BOOL sid_load(FIL *f, BYTE song)
{
     
    static WORD i,m,n,e,s,d;
    static WORD bytes_read;
    static FRESULT res;
    static DWORD *magic;
    static WORD player;
    
    WORD  *offset;
    BYTE   tmp;
            
    res = f_lseek(f, 0L);
	if(res != FR_OK) return FALSE;

    res = f_read(f, sid_header, 0x7E, &bytes_read);
	if(res != FR_OK) return FALSE;
    
    // header checks
    magic = (DWORD *)sid_header;
    
    if((*magic != magic_rsid)&&(*magic != magic_psid)) {
        printf("Filetype not as expected. (%08lx)\n", *magic); 
        return FALSE;
    }
    
    SWAP(tmp, sid_header[ 4], sid_header[ 5]);
    SWAP(tmp, sid_header[ 6], sid_header[ 7]);
    SWAP(tmp, sid_header[ 8], sid_header[ 9]);
    SWAP(tmp, sid_header[10], sid_header[11]);
    SWAP(tmp, sid_header[12], sid_header[13]);
    SWAP(tmp, sid_header[14], sid_header[15]);
    SWAP(tmp, sid_header[16], sid_header[17]);

    if(song == 0xFF) { // play default song
        song = sid_header[0x10]; // little endian now
        if(!song)
            song = 1;
    }

    sid_header[0x10] = song-1;

    // reset file pointer to C64 data
    offset = (WORD *)&sid_header[6];
    f_lseek(f, *offset);

    if(!sid_load_init()) {
        printf("Handshake failed.\n");
        return FALSE;
    }
    
    // load file into C64 memory
    res = f_read(f, &s, 2, &bytes_read);

    printf("Start address of .SID = %04x.\n", s);
    e = s;
    m = (s >> 13) | MAP_RAMDMA;
    i = s & 0x1FFF;
    n = 0x2000 - i;
    i |= MAP1_ADDR;
    
    while(bytes_read) {
        MAPPER_MAP1 = m;
        f_read(f, (void *)i, n, &bytes_read);
        printf("%d bytes read to addr %04x. Requested %04x bytes.\n", bytes_read, i, n);
        e += bytes_read;
        i = MAP1_ADDR;
        n = 0x2000;
        m ++;
    }

    *(WORD *)(&sid_header[0x7E]) = e; // store end address in header for our friend Wilfred

    // now decide on where to put the player
    // if the player is version 1, then we'll just choose something
    if (sid_header[4] < 0x02) {
        if (s >= 0x1000) {
            player = 0x0800;
        } else if(e <= 0xCF00) {
            player = 0xCF00;
        } else {
            player = 0x0400;
        }
    } else {
        // version 2 and higher
        if(sid_header[0x78] == 0xFF) {
            printf("No reloc space.\n");
            dma_load_exit(DMA_BASIC);
            return FALSE;
        }
        if(sid_header[0x78] == 0x00) {
            printf("Clean SID file.. checking start/stop\n");
            if (s >= 0x1000) {
                player = 0x0800;
            } else if(e <= 0xCF00) {
                player = 0xCF00;
            } else {
                player = 0x0400;
            }
        } else {
            if(sid_header[0x79] < 1) {
                printf("Space for driver too small.\n");
                dma_load_exit(DMA_BASIC);
                return FALSE;
            }
            player = ((WORD)sid_header[0x78]) << 8;
        }            
    }

    printf("Player address: %04x.\n", player);

    m = (player >> 13) | MAP_RAMDMA;
    i = player & 0x1FFF;
    i |= MAP1_ADDR;
    
    // copy the header
    MAPPER_MAP1 = m;
    memcpy((char *)i, sid_header, 0x80);

/*
    // copy another 0x300 bytes (arbitrary, t.b.d.)
    MAPPER_MAP2 = 4096;  // use DRAM base to load player from (this memory is filled with myrom.bin (cart))
    s = MAP2_ADDR;

    n = *(WORD *)MAP2_ADDR; // read length field
    if(n > 0x780)
        n = 0x780; // limit
        
    printf("Going to copy %04x bytes.\n", n);
    
//    s = (WORD)&sid_player[0];
    d = player + 0x80;   // copy to player address + 0x80

    do {
        m = (d >> 13) | MAP_RAMDMA;
        i = d & 0x1FFF;
        i |= MAP1_ADDR;
        MAPPER_MAP1 = m;
        memcpy((char *)i, (char *)s, 0x80);
        printf("Memcpy: %04x(%04x) %04x.\n", i, m, s);
        s += 0x80;
        d += 0x80;
        if(n >= 0x80)
            n -= 0x80;
        else
            n = 0;
    } while(n);
*/
    // set jump address to player + 0x80
    MAPPER_MAP1 = MAP_RAMDMA;
    *(WORD *)(MAP1_ADDR + 0x0144) = player; // + 0x82;

    printf("Set vector to: %04x.\n", *(WORD *)(MAP1_ADDR + 0x0144));

    // send end address of load
    MAPPER_MAP1 = MAP_RAMDMA;
    *(WORD *)(MAP1_ADDR + 0x002D) = e;
    *(WORD *)(MAP1_ADDR + 0x002F) = e;
    *(WORD *)(MAP1_ADDR + 0x0031) = e;
    *(WORD *)(MAP1_ADDR + 0x00AE) = e;
    *(BYTE *)(MAP1_ADDR + 0x0002) = song;

//    // copy header to 0x340
//    memcpy((char *)MAP1_ADDR + 0x0340, sid_header, 0x7c);

    sid_load_exit();
}
