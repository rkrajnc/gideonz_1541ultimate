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
 *	 Exports of the functions that handle screen output
 */
#ifndef SCREEN_H
#define SCREEN_H

#include "types.h"

void scroll_down();
void scroll_up();
void scroll_left();
void char_out(BYTE b);
void clear_screen(BYTE color);  // negative: don't set color
void set_color(BYTE color);     // fill screen with color

extern WORD start_pos;
extern BYTE num_lines;
extern BYTE width;
extern BYTE do_color;
extern BYTE clear_line;
extern BYTE cursor_x;
extern BYTE cursor_y;
extern WORD cursor_pos;
extern char scroll_char;

#pragma zpsym ("cursor_x");
#pragma zpsym ("cursor_y");
#pragma zpsym ("cursor_pos");

#endif