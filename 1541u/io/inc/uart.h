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
 *	 Exported items allowing the handling of the uart.
 */
#ifndef UART_H
#define UART_H

#include "types.h"


#define UART_DATA  *((BYTE *)0x2100)
#define UART_GET   *((BYTE *)0x2101)
#define UART_FLAGS *((BYTE *)0x2102)
#define UART_ICTRL *((BYTE *)0x2103)

#define UART_Overflow    0x01
#define UART_TxFifoFull  0x10
#define UART_RxFifoFull  0x20
#define UART_TxReady     0x40
#define UART_RxDataAv    0x80

SHORT uart_read_buffer(const void *buf, USHORT count);
SHORT uart_write_buffer(const void *buf, USHORT count);
BOOL uart_data_available(void);
void uart_hex(BYTE); // fast routine to push a hex value in the fifo, no checks!
 // defined in spi.asm
#endif
