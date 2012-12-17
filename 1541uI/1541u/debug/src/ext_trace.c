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
 *	 Utility to convert a binary dump of a trace into VCD format
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../../data/inc/types.h"

struct data_entry {
    WORD stamp;
    WORD data;
    WORD aux;
};


static void print_binary (uint64_t val, int bits)
{
  int bit;
  BOOL leading = TRUE;
  while (--bits >= 0)
    {
      bit = ((val & (1LL << bits)) != 0LL);
      if (leading && (bits != 0) && ! bit)
	continue;
      leading = FALSE;
      fputc ('0' + bit, stdout);
    }
}

void dump_trace(FILE *fi)
{
    DWORD time = 0;
    WORD  prev, current, change;
    int   r,i,z;
    
    struct data_entry d;
        
    const char vcd_header[] = "$timescale\n 1 us\n$end\n\n";
    const char vcd_middle[] = "\n$enddefinitions $end\n\n#0\n$dumpvars\n";

    const char *labels[16] = { "state(0)", "state(1)", "state(2)", "state(3)", "data_av", "drv_atn", "drv_data", "drv_clk", 
                               "sw_ready", "cart_atn", "cart_data", "cart_clk", "bus_srq", "bus_atn", "bus_data", "bus_clk" };
    BYTE   b;
        
    z = 6;

    printf(vcd_header);
    
    for(b=0;b<16;b++) {
        if(*labels[b])
            printf("$var wire 1 %c %s $end\n", 65+b, labels[b]);
    }    
    if(z == 6)
        printf("$var wire 16 @ pc_1541[15:0] $end\n");
      
    printf(vcd_middle);
    
    for(i=0;;i++) {
        r = fread(&d, z, 1, fi);
        if(r != 1)
            break;
        if(d.aux == 0xAAAA)
            break;

        if(i!=0)
            time += (d.stamp & 0x7FFF) + 1;
        printf("#%ld\n", time);
        current = d.data;
        change = current ^ prev;
        for(b=0;b<16;b++) {
            if((change & 1)||(i==0)) {
                printf("%c%c\n", ((current >> b) & 1)+48, 65+b);
            }
            change >>= 1;
        }
        if(z == 6) {
            printf("b");
            print_binary((uint64_t)d.aux, 16);
            printf(" @\n");
        }
        prev = current;
    }
}

int main(int argc, char **argv)
{
    FILE *fi;
    
    if(argc != 2) {
        printf("Usage: dump_vcd <file>\n");
        exit(1);
    }
    fi = fopen(argv[1], "rb");
    if(fi) {
        dump_trace(fi); 
    } else {
        fprintf(stderr, "Can't open file.\n");
        exit(2);
    }
    fclose(fi);
    return 0;
}
