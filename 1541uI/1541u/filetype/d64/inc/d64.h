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
 *	 Functions that handle D64/D71 and D81 file formats.
 */
#ifndef D64_H
#define D64_H

#include "ff.h"
#include "dir.h"

#define D64_SIZE   0x2AB00
#define D64_BAMLOC 0x16500 

FRESULT d64_dir(FIL *d64_file, DIRECTORY* directory, BOOL ascii_only, BYTE mode);
FRESULT d64_readblock(FIL *d64_file, BYTE *block, BYTE *len, DWORD start, DWORD *next, BYTE mode);
FRESULT d64_loadfile(FIL *d64_file, DWORD start, BOOL runflag, BYTE mode);
FRESULT d64_write_track(FIL *mountfile, BYTE tr);
FRESULT d64_create(FIL *f, char *name);
FRESULT d64_save(FIL *f, char *error);
void    d64_load_params(void);

#endif
