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
 *   Exports of the functions that implement the 1541 Ultimate menu
 *   on the C64 screen.
 */

#ifndef C64_MENU_H
#define C64_MENU_H

#include "types.h"

void menu_reset_global_scroll(void);
void do_scroller(void);
void menu_init(void);
BOOL menu_run(BYTE);
void draw_menu(void);
void draw_directory(void);
void set_drive_addr(BYTE);
void handle_create_disk(void);
void handle_create_dir(void);
void handle_dump_mem(void);
void handle_flash(void);
void prg_loadfile(BOOL);
BOOL string_box(WORD pos, char w, char *buffer, BYTE max_len);



//////////////////////////////////
///	Stand ALone (sal) MENU STUB //
//////////////////////////////////
void sal_handle_up(void);
void sal_handle_down(void);
BOOL sal_handle_select(BOOL);

#define NUM_COLMS	38
#define NUM_LINES   21
#define MAX_DEPTH   12

#define SCROLL_OUT    21
#define COP_POS_BRDR   7
#define COP_POS_SCR    9
// are now defined in copper.asm, because they are needed there (dirty)
//#define COP_POS_Y1     5
//#define COP_POS_Y2     11
//#define COP_POS_SCROLL 19


#endif
