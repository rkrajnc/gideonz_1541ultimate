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
 *	 Version information for the 1541 Ultimate Application and hardware
 *   type definitions.
 *
 */

#ifndef _VERSION_H
#define _VERSION_H

#include "types.h"

//#define SW_VERSION "V1.5\021" // beta sign
#define SW_VERSION "V1.7\021"


#define HW_BASIC    0
#define HW_PLUS     1
#define HW_ETHERNET 2

#define HW_BIT_32MB  0x02
#define HW_BIT_ETH   0x04

#define HW_MSK_BASIC 0x01
#define HW_MSK_PLUS  (HW_MSK_BASIC | HW_BIT_32MB)
#define HW_MSK_ETH   (HW_MSK_PLUS | HW_BIT_ETH)

void version_init(void);

extern BYTE hardware_type;
extern BYTE hardware_mask;

#endif
