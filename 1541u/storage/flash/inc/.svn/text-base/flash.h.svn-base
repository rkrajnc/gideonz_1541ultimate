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
 *	 Functions for handling flash memory
 *
 */

#ifndef __FLASH_H
#define __FLASH_H

#include "types.h"

BYTE read_hexbyte(FIL *fp);
BOOL flash_erase_block(WORD d);
BOOL flash_erase(void);
BOOL flash_file(FIL *fp, BYTE v);
WORD flash_get_cfg_page(void);
WORD flash_get_cfg_sector(void);
void flash_page(WORD, WORD);  // is located in the ASM file
WORD verify_page(WORD, WORD);  // is located in the ASM file
WORD flash_addr2block(LONG addr);
void flash_protect(void);

#endif
