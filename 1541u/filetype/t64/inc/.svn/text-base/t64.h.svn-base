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
 *	 Functions that handle the T64 file format.
 */
#ifndef T64_H
#define T64_H

FRESULT t64_dir(FIL *t64_file, DIRECTORY* directory, BOOL ascii_only);
FRESULT t64_readblock(FIL *t64_file, BYTE *block, BYTE *len, DWORD start, DWORD *next);
FRESULT t64_loadfile(FIL *t64_file, DWORD start, WORD c64_strt, WORD length, BOOL runflag);
FRESULT t64_loadfirst(FIL *t64_file, BOOL runflag);

#endif
