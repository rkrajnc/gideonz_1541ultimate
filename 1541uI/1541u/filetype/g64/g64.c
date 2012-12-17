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
 *	 Handler / loader of the G64 file format.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "ff.h"
#include "dir.h"
#include "mapper.h"
#include "gcr.h"
#include "gpio.h"

//#pragma codeseg ("FILESYS")

#define PARAM_BASE 0x2C00  // this should move to some header file

ULONG track_offsets[84];

BYTE read_mem(ULONG address)
{
    MAPPER_MAP1 = (WORD)(address >> 13);
    return *((BYTE *)(MAP1_ADDR + (address & 0x1FFF)));
}


BYTE load_g64(FIL *fp, BYTE wp, BYTE delay)
{
    // first just load the whole damn thing in memory, up to $50000 in length
    static WORD w;
    static ULONG total;
    static WORD bytes_read;
    static ULONG *pul;
    static ULONG *param;
    static BYTE b;
    static FRESULT fres;
    
    GPIO_SET = WRITE_PROT;
    total = 0;
    
    for(w=GCR_ADDR;w<GCR_END_ADDR;w++) {
        MAPPER_MAP1 = w;
        fres = f_read(fp, (void *)0x4000, 8192, &bytes_read);
        total += bytes_read;
        if(bytes_read != 8192)
            break;
    }
    printf("Total bytes read: %ld.\n", total);
    
    // check signature
    MAPPER_MAP1 = GCR_ADDR;
    pul = (ULONG *)0x4000;
    if((pul[0] != 0x2D524347)||(pul[1] != 0x31343531)) { // little endianness assumed
        printf("Wrong header.\n");
        return 0;
    }
    
    param = (ULONG *)PARAM_BASE;

    // extract parameters
    // track offsets start at 0x000c
    for(b=0;b<84;b++) {
        track_offsets[b] = pul[b+3] + (GCR_ADDR << 13);
        param[b<<1] = track_offsets[b] + 2;
    }
    for(b=0;b<84;b++) {
//        w = 1;
        if(track_offsets[b] != (GCR_ADDR << 13)) {
            w = read_mem(track_offsets[b]);
            w |= ((WORD)read_mem(track_offsets[b]+1)) << 8;
            printf("Set track %d.%d to 0x%06lX / 0x%04X.\n", (b>>1)+1, (b&1)?5:0, track_offsets[b] + 2, w);
        } else {
            printf("Skipping track %d.%d.\n", (b>>1)+1, (b&1)?5:0);
        }
        param[(b<<1) + 1] = w - 1;
    }
    printf("Done.\n");
    
    disk_swap(wp, delay);
    return 1;
}
