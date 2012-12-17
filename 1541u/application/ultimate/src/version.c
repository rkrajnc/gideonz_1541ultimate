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
 *	 Version information - This module checks for hardware type.
 *
 */
#include <stdio.h>
#include "version.h"
#include "onewire.h"

BYTE hardware_type;
BYTE hardware_mask;
BYTE serial[8];

void version_init(void)
{
    BYTE b;

//    hardware_type = HW_BASIC;
//    hardware_mask = HW_MSK_BASIC;
    hardware_type = HW_PLUS;
    hardware_mask = HW_MSK_PLUS;
    
    if(onewire_readrom(serial)) {
        for(b=0;b<8;b++) {
            printf("%02X ", serial[b]);
        } printf("\n");
        b = onewire_readmem(0x60);
        printf("b=%02X\n", b);
        if(b == 0xFF) {
            hardware_type = HW_PLUS;
            hardware_mask = HW_MSK_PLUS;
        } else if(b == 0x7F) {
            hardware_type = HW_ETHERNET;
            hardware_mask = HW_MSK_ETH;
        }
        b = onewire_readmem(0x62);
        if(b == 0xAA) { // upgraded boards
            hardware_type = HW_ETHERNET;
            hardware_mask = HW_MSK_ETH;
        }
    } else {
        printf("Onewire device not present.\n");
    }
}
