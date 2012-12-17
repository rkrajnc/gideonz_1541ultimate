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
 *	 Routine to dump the stored trace in "verilog" value change dump format.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mapper.h"

static DWORD get_trace_entry(int i)
{
    static DWORD *pnt = (DWORD *)0x2800;
    
    if(i & 0x200) {
        MAPPER_MAP3L = 1;
    } else {
        MAPPER_MAP3L = 0;
    }
    i &= 0x1FF;
    return pnt[i];
}

void dump_trace(void)
{
    static DWORD w;
    static DWORD time = 0;
    static WORD prev, current, change;
    
    const char vcd_header[] = "$timescale\n 1 us\n$end\n\n";
    const char vcd_middle[] = "\n$enddefinitions $end\n\n#0\n$dumpvars\n";
    const char *labels[16] = { "dbg(0)", "dbg(1)", "dbg(2)", "dbg(3)", "cart_srq", "drv_atn", "drv_data", "drv_clk", 
                               "cart_auto", "cart_atn", "cart_data", "cart_clk", "bus_srq", "bus_atn", "bus_data", "bus_clk" };
    int    i,len;
    int    start = 0;
    BYTE   b;
        
    for(i = 0;i<1024;i++) {
        if(get_trace_entry(i) == 0xFFFFFFFF) {
            start = (i + 1) & 0x3FF;
            break;
        }
    }
    if(get_trace_entry(start) == 0L) {
        len = start - 1;
        start = 0;
    } else {
        len = 1023;
    }

    printf("start = %d. len = %d.\n", start, len);
    
    printf(vcd_header);
    
    for(b=0;b<16;b++) {
        if(*labels[b])
            printf("$var wire 1 %c %s $end\n", 65+b, labels[b]);
    }    
  
    printf(vcd_middle);
    
    for(i=0;i<len;i++) {
        w = get_trace_entry((start + i) & 0x3FF);
        time += (w & 0x7FFF);
        printf("#%ld\n", time, w);
        current = (w >> 16);
        change = current ^ prev;
        for(b=0;b<16;b++) {
            if((change & 1)||(i==0)) {
                printf("%c%c\n", ((current >> b) & 1)+48, 65+b);
            }
            change >>= 1;
        }
        prev = current;
    }
}
