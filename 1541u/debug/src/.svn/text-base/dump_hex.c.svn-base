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

#pragma codeseg ("LIBCODE")

#include <stdio.h>
#include "dump_hex.h"

#ifndef DUMP_BYTES
#define DUMP_BYTES 16
#endif

void dump_hex(BYTE *p, WORD len)
{
    WORD w,t;
    BYTE c;
    
	for(w=0;w<len;w+=DUMP_BYTES) {
		printf("%04x: ", (WORD)p + w);
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