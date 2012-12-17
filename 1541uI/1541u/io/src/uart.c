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
 *	 Functions handling the uart
 */
#include "manifest.h"
#include <stdio.h>
#include "uart.h"
#include "soft_signal.h"

#define VT100_CR 0x0D
#define VT100_NL 0x0A

/*
-------------------------------------------------------------------------------
							uart_data_available
							===================
  Abstract:

	Returns if something is available from the uart

  Parameters
	hnd:		unused
	buf:		pointer to buffer receiving data
	count:		number of bytes to read

  Return:
	BOOL:		FALSE if not
	others:		if so
-------------------------------------------------------------------------------
*/
BOOL uart_data_available(void)
{
	return (UART_FLAGS & UART_RxDataAv);
}

/*
-------------------------------------------------------------------------------
							uart_read_buffer
							================
  Abstract:

	Reads from the uart

  Parameters
	hnd:		unused
	buf:		pointer to buffer receiving data
	count:		number of bytes to read

  Return:
	i:		number of bytes read
-------------------------------------------------------------------------------
*/
SHORT uart_read_buffer(const void *buf, USHORT count)
{
    volatile BYTE st;
    BYTE *pnt = (BYTE *)buf;
    SHORT i = 0;
    BYTE d;

    clear_signal(SGNL_UART_BREAK);
	    
	d = 0;   
    do {
        st = UART_FLAGS;
        if (st & UART_RxDataAv) {
            if (count) {
                d = UART_DATA;
               	*(pnt++) = d;
                UART_GET = 0;
                count --;
                i++;
            }
        } else {
            //break; // only break for FILES, not for uart. Break on newline
        }
        d &= 0x7F;
    } while(count && (d != VT100_CR) && (d != VT100_NL) && !signal_set(SGNL_UART_BREAK));
//    UART_DATA = 0x2E;
    
    return i;
}

/*
-------------------------------------------------------------------------------
							uart_write_buffer
							=================
  Abstract:

	Writes to the uart

  Parameters
	hnd:		unused
	buf:		pointer to buffer containing data to write
	count:		number of bytes to write

  Return:
	i:		number of bytes read
-------------------------------------------------------------------------------
*/
SHORT uart_write_buffer(const void *buf, USHORT count)
{
    SHORT i;
    volatile BYTE st;
    BYTE *pnt = (BYTE *)buf;

    for(i=0;i<count;i++) {
        do {
            st = UART_FLAGS;
        }
//        while(st & UART_TxFifoFull);
        while(!(st & UART_TxReady));

        if(*pnt == 0x0A) {
            UART_DATA = 0x0D;
        }

		if(*pnt == 0x0D) {
	        UART_DATA = *pnt++;
            UART_DATA = 0x0A;
			continue;
        }
        UART_DATA = *pnt++;
    }

    return count;
}


