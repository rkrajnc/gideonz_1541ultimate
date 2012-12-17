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
 *   Functions to handle the emulation of C64 cartridges.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpio.h"
#include "config.h"
#include "cartridge.h"
#include "version.h"

extern const char *cart_mode[];
extern BYTE cart_types[];
//extern BYTE cart_bases_pal[];
//extern BYTE cart_bases_ntsc[];
extern BYTE cart_bases[];
extern BYTE plus_version;
static BYTE glob_cart_mode;
static BYTE glob_cart_base;
static BYTE glob_eth;

BYTE get_cart_base(void)
{
    BYTE    cartridge_select;
    BYTE    base;
    cartridge_select = (BYTE)cfg_get_value(CFG_CART_MODE);
    base = cart_bases[cartridge_select];

    return base;
}

/* cannot be called from file system */
void init_cartridge(void)
{
    BYTE    cartridge_select;
//    BYTE    ntsc;
    
    CART_MODE = 0x00; // force change to reenable
    
    cartridge_select = (BYTE)cfg_get_value(CFG_CART_MODE);
/*

    ntsc             = (BYTE)cfg_get_value(CFG_PAL_NTSC);

//	printf("card select: #%d:%s (0x%02x)\n", cartridge_select, cart_mode[cartridge_select], cart_setting[cartridge_select]);

    if(ntsc)
        glob_cart_base = cart_bases_ntsc[cartridge_select];        
    else
        glob_cart_base = cart_bases_pal[cartridge_select];        
*/
    glob_cart_base = cart_bases[cartridge_select];
    
    if((hardware_mask & HW_BIT_32MB) && (cfg_get_value(CFG_REU_ENABLE)))
        glob_cart_mode = cart_types[cartridge_select];
    else
        glob_cart_mode = cart_types[cartridge_select] & 0x7F;

    if((hardware_mask & HW_BIT_ETH) && (cfg_get_value(CFG_ETH_ENABLE)) && (glob_cart_mode & 0x40)) {
		glob_eth = 1;
	} else {
		glob_eth = 0;
	}

    enable_cartridge();
}

/* can be called from file system! */

#pragma codeseg ("DMACODE")

void enable_cartridge(void)
{
    CART_MODE = glob_cart_mode;
    CART_BASE = glob_cart_base;

    // ethernet
    if(glob_eth) {
        CONFIG_ETH_EN = 1;
    } else {
        CONFIG_ETH_EN = 0;
    }

//    printf("Setting the register to %02x. because pv=%02x.\n", glob_cart_mode, plus_version);
}

void disable_cartridge(void)
{
    CART_MODE = 0;  // turn off cartridges
    printf("Turning off cartridges..\n");
}
