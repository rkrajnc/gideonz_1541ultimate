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
 *	 Exports of the one-wire protocol functions
 */
#ifndef ONEWIRE_H
#define ONEWIRE_H

BYTE onewire_reset(void);
void onewire_sendbyte(BYTE byte);
BYTE onewire_getbyte(void);
BYTE onewire_readrom(BYTE *rom);
BYTE onewire_readmem(BYTE offset);
BOOL onewire_progbuf(BYTE offset, char *buffer, BYTE len);
void onewire_waitprog(void);

#endif
