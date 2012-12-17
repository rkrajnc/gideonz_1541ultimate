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
 *   Function to interpret logic analyzer data of the C64 cartridge port.
 *   (Obsolete)
 */
#include "types.h"

#define ANALYZER_DATA(x) *((BYTE*)0x2800+x)
#define ANALYZER_SELECT  *((BYTE*)0x2800)
#define ANALYZER_CTRL    *((BYTE*)0x2801)

#define ANALYZER_ALL_EDGES    0x00
#define ANALYZER_RISING_EDGE  0x01
#define ANALYZER_FALLING_EDGE 0x02
#define ANALYZER_LOW_LEVEL    0x03
#define ANALYZER_C64_ONLY     0x10
#define ANALYZER_FALLING_PHI2 0x20

WORD timing_data[24][52];

void test_timing(void)
{
    BYTE i,j,k;
    

    for(i=0;i<24;i++) {
        ANALYZER_SELECT = i;
        
        // clear array
        for(j=0;j<52;j++) {
            timing_data[i][j] = 0;
        }
        
        // do test
        for(j=0;j<16;j++) {
            ANALYZER_CTRL = ANALYZER_C64_ONLY | ANALYZER_ALL_EDGES | ANALYZER_FALLING_PHI2;

            while(ANALYZER_DATA(60))
                ;
            
            printf(".");
            for(k=0;k<52;k++) {
                timing_data[i][k] += ANALYZER_DATA(k);
            }
        }
        printf("\n");
    }
    printf("Report:\n");
    for(k=0;k<52;k++) {
        for(i=0;i<24;i++) {
            printf("%d,", timing_data[i][k]);
        }
        printf("\n");
    }
    printf("End of report.\n");
}

