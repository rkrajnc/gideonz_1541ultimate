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
 */
#ifndef __DATA_TOOLS_H
#define __DATA_TOOLS_H

#include "types.h"


#ifdef NO_FLOAT
#define MAXDIG      11  /* 32 bits in radix 8 */
#else
#define MAXDIG      128 /* this must be enough */
#endif

unsigned data_atoi(CHAR *p);
ULONG data_atol(CHAR *p);

CHAR *data_itoa(CHAR *p, unsigned i, unsigned radix);
CHAR *data_ltoa(CHAR *p, ULONG i, unsigned radix);
BYTE data_strnlen(CHAR *p, BYTE max);
BYTE data_strncpy(CHAR *p1, CHAR *p2, BYTE max);
BYTE data_strnicpy(CHAR *p1, CHAR *p2, BYTE max); // fixes case

#endif
