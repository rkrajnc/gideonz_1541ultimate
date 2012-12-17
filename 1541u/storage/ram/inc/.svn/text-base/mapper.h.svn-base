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
 *	 Definitions allowing for readable use of the 65gz02 memory mapper.
 *
 */
#ifndef MAPPER_H
#define MAPPER_H

#include "types.h"

#define MAPPER_MAP1   *((WORD*)0x2300)
#define MAPPER_MAP2   *((WORD*)0x2302)
#define MAPPER_MAP3   *((WORD*)0x2304)
#define MAPPER_CODE   *((WORD*)0x2306)   /* 1 byte only */

#define MAPPER_MAP1L  *((BYTE*)0x2300)
#define MAPPER_MAP1H  *((BYTE*)0x2301)
#define MAPPER_MAP2L  *((BYTE*)0x2302)
#define MAPPER_MAP2H  *((BYTE*)0x2303)
#define MAPPER_MAP3L  *((BYTE*)0x2304)
#define MAPPER_MAP3H  *((BYTE *)0x2305)

#define MAP1_ADDR   0x4000
#define MAP2_ADDR   0x6000
#define MAP3_ADDR   0x3000

#define IEC_DIR_ADDR    48  /* (0x54000 >> 13) */
#define MENU_DIR_ADDR   49  /* (0x56000 >> 13) */
#define ROM1541_ADDR    50  /* (0x64000 >> 13) */
#define APPL_ADDR       52  /* (0x68000 >> 13), extends up to 7FFFF, with a gap from 70000-71FFF */
#define CHARS_ADDR      56  /* (0x70000 >> 13) */
#define CUST_CART_ADDR  4096
#define KCS_ADDR        4104
#define GCR_ADDR        4128
#define GCR_END_ADDR    4216

//#define MAP_VICDMA  0x0406 // 80c000
#define MAP_RAMDMA    0x0400 //  800000
#define MAP_FLASH     0x0800 // 1000000
#define MAP_SDRAM_CMD 0x0C00 // 1800000
#define MAP_SDRAM     0x1000 // 2000000
#define MAP_BACKUP    56     //  070000

#define MAP3_IO       0x080D // 80d000

/* WARNING: MAP 3 is only 4K, which means that the map addresses need to be
            shifted one position to the LEFT still. Example: Using map 3
            to 'see' the first 4K of the 1541, the value written should be
            100, not 50!!  */
#endif
