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
 *	 Simple routine to dump some hex values on the console.
 *
 */

#include "integer.h"
#include "small_printf.h"

#ifndef DUMP_BYTES
#define DUMP_BYTES 16
#endif

void dump_hex_actual(void *pp, int len, int relative)
{
    int w,t;
    BYTE c;
    BYTE *p = (BYTE *)pp;
    
	for(w=0;w<len;w+=DUMP_BYTES) {
        if(relative)
            printf("%4x: ", w);
        else
		    printf("%p: ", p + w);
        for(t=0;t<DUMP_BYTES;t++) {
            if((w+t) < len) {
		        printf("%02x ", p[w+t]);
		    } else {
		        printf("   ");
		    }
		}
        for(t=0;t<DUMP_BYTES;t++) {
            if((w+t) < len) {
                c = p[w+t];
                if((c >= 0x20)&&(c <= 0x7F)) {
                    printf("%c", c);
                } else {
                    printf(".");
                }
		    } else {
		        break;
		    }
		}
		printf("\n");
	}
}

void dump_hex(void *pp, int len)
{
    dump_hex_actual(pp, len, 0);
}

void dump_hex_relative(void *pp, int len)
{
    dump_hex_actual(pp, len, 1);
}

void dump_hex_dirty(void *pp, int len, BYTE ptrn)
{
	int w,t,d;
    BYTE *p = (BYTE *)pp;
	for(w=0;w<len;w+=DUMP_BYTES) {
		for(d=0,t=0;t<DUMP_BYTES;t++) {
			if(p[w+t] != ptrn)
				d++;
		}
		if(d) {
			dump_hex(&p[w], DUMP_BYTES);
		}
	}
}

