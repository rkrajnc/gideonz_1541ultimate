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
 *   Soft-Signaling functions
 */

#ifndef SOFT_SIGNAL_H
#define SOFT_SIGNAL_H

/*
void set_signal(unsigned char b);
void clear_signal(unsigned char b);
unsigned char signal_set(unsigned char b);
*/


#define NUM_SIGNALS 8
#define SGNL_UART_BREAK 0x01

extern unsigned char Signals[NUM_SIGNALS];

#define set_signal(a)   Signals[a] = 1;
#define clear_signal(a) Signals[a] = 0;
#define signal_set(a)   Signals[a]

#endif