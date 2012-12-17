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
 *	 Exported items allowing the handling of the c2n playback unit.
 */

#ifndef C2N_H
#define C2N_H

#include "types.h"

#define C2N_FIFO_ADDR 0x2600
#define C2N_CONTROL   *((BYTE *)0x2400)
#define C2N_STATUS    *((BYTE *)0x2400)

#define C2N_ENABLE          0x01
#define C2N_ERROR           0x02
#define C2N_FIFO_FULL       0x04
#define C2N_FIFO_ALMOSTFULL 0x08
#define C2N_FIFO_EMPTY      0x80

#endif
