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
 *   Routines to do fast operations on memory.
 */
#include "types.h"

#define BLRAM 	0x0000
#define PERIP 	0x2000
//#define IECDEB 	0x3000
#define MAP1 	0x4000
#define MAP2 	0x6000
#define APPL 	0x8000

void   ClearFloppyMem(void);
void   SDRAM_Init(void);
WORD   TestPage(BOOL, WORD);
WORD   FillPage(BYTE, WORD);
WORD   copy_page(WORD, WORD);
void   memswap(void *a, void *b, BYTE len);

/*
extern BYTE stackframe[24];
extern WORD debug_loc1;
extern WORD debug_loc2;
*/

