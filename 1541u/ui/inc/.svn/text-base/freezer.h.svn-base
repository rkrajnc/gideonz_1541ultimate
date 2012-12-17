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
 *   Exports of functions that initiate the freezing and can exit the
 *   freezer gracefully
 */
#ifndef FREEZER_H
#define FREEZER_H

#include "types.h"

// bit7: exit
// bit4: go to basic
// bit1: go to rsid

#define DMA_RUN    0x80
#define DMA_BASIC  0xC0
#define DMA_RSID   0xC1

void stop_c64(BOOL);
void run_c64(void);
void backup_c64_io(void);
void init_c64_io(void);
void restore_c64_io(void);
void init_freeze(void);
void exit_freeze(void);
void dma_load_init(void);
void dma_load_exit(BYTE run_code);

#endif
