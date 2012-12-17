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
 *   Definitions of C64 memory (IO) locations
 */
#ifndef C64_ADDR
#define C64_ADDR

#include "mapper.h"

// MAP1 locations (pointing to C64 RAM)
#define SCREEN1_ADDR (MAP1_ADDR + 0x0400)

// MAP3 locations (pointing to C64 I/O)
#define COLOR3_ADDR    (MAP3_ADDR + 0x0800)
#define COLOR_MAP3(x) *((unsigned char *)(MAP3_ADDR + 0x0800 + x))
#define BORDER        *((unsigned char *)(MAP3_ADDR + 0x0020))
#define BACKGROUND    *((unsigned char *)(MAP3_ADDR + 0x0021))
#define VIC_CTRL      *((unsigned char *)(MAP3_ADDR + 0x0011))
#define VIC_MMAP      *((unsigned char *)(MAP3_ADDR + 0x0018))
#define VIC_REG(x)    *((unsigned char *)(MAP3_ADDR + x))
#define VIC_ISR       *((unsigned char *)(MAP3_ADDR + 0x0019))
#define SID_VOLUME    *((unsigned char *)(MAP3_ADDR + 0x0418))
#define SID_DUMMY     *((unsigned char *)(MAP3_ADDR + 0x041F))
#define CIA2_DPA      *((unsigned char *)(MAP3_ADDR + 0x0D00))
#define CIA2_DDRA     *((unsigned char *)(MAP3_ADDR + 0x0D02))
#define CIA1_DPA      *((unsigned char *)(MAP3_ADDR + 0x0C00))
#define CIA1_DDRA     *((unsigned char *)(MAP3_ADDR + 0x0C02))
#define CIA1_DDRB     *((unsigned char *)(MAP3_ADDR + 0x0C03))
#define CIA1_ICR      *((unsigned char *)(MAP3_ADDR + 0x0C0D))
#define CIA2_ICR      *((unsigned char *)(MAP3_ADDR + 0x0D0D))

#endif