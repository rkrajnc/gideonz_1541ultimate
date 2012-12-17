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
 *	Exported items regarding;
 *	Converting D64 images to GCR and loading it to the emulated 1541's buffer
 *
 */

#ifndef GCR_H
#define GCR_H

#include "ff.h"
#include "types.h"

BYTE load_d64(FIL *, BYTE wp, BYTE delay);  // now returns the number of tracks read
BYTE load_g64(FIL *, BYTE wp, BYTE delay);  // 
BYTE disk_swap(BYTE wp, BYTE delay);
WORD find_trackstart(void);
BYTE *get_sector(BYTE track);
BYTE get_num_sectors(BYTE track, BYTE mode);
WORD ts2sectnr(BYTE s, BYTE t, BYTE mode);
extern BYTE sector_buf[260];
extern BYTE header[8];

#endif
