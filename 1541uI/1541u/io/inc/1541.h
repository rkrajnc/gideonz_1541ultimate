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
 *   Definitions of the software regsiters that interface with the 1541
 */

#ifndef _1541_H
#define _1541_H

#define TRACK_DIRTY(x) *((BYTE *)(0x2500 + x)) // clear on write
#define ANY_DIRTY      *((BYTE *)0x2540) // clear on write
#define DIRTY_IRQ_EN   *((BYTE *)0x2541) // bit 0 = irq enable on any dirty
#define DRV_STATUS     *((BYTE *)0x2542) // read only
#define DRV_TRACK      *((BYTE *)0x2543) // read only
 
#define DRV_WRITE 0x01
#define DRV_MOTOR 0x02

#endif
