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
 *	 Functions for converting data. Basically generic functions
 *
 */
#include "manifest.h"
#include "data_tools.h"

#if 0
/*
-------------------------------------------------------------------------------
							data_atoi
							=========
  Abstract:

	converts ascii HEX string to value. (uses the default type)

  Parameters
	line_in:		Pointer pointing to the start of the string
					
  Return:
	value:		32bits value
-------------------------------------------------------------------------------
*/
unsigned data_atoi(CHAR *p)
{
    unsigned i = 0;
    BYTE b = 0;
	
	// While not ran out of string
	while(p[b] != 0) { 
		i <<= 4;
		if((p[b] >= '0')&&(p[b] <= '9'))      i |= (p[b] & 0x0F);
	    else if((p[b] >= 'A')&&(p[b] <= 'F')) i |= (p[b] - 55);
	    else if((p[b] >= 'a')&&(p[b] <= 'f')) i |= (p[b] - 87);
	    else break;
		b++;
	}
	i >>= 4;

    return i;
}
#endif
/*
-------------------------------------------------------------------------------
							data_atol
							=========
  Abstract:

	converts ascii HEX string to value. (uses the long type)

  Parameters
	line_in:		Pointer pointing to the start of the string
					
  Return:
	value:		32bits value
-------------------------------------------------------------------------------
*/
ULONG data_atol(CHAR *p)
{
    ULONG i = 0L;
    BYTE b = 0;
	
	// While not ran out of string
	while(p[b] != 0) { 
		i <<= 4;
		if((p[b] >= '0')&&(p[b] <= '9'))      i |= (p[b] & 0x0F);
	    else if((p[b] >= 'A')&&(p[b] <= 'F')) i |= (p[b] - 55);
	    else if((p[b] >= 'a')&&(p[b] <= 'f')) i |= (p[b] - 87);
	    else break;
		b++;
	}
//	i >>= 4;

    return i;
}

/*
-------------------------------------------------------------------------------
							data_strnlen
							============
  Abstract:

	finds the string length but takes a limit into account for whemn there is 
	no zero termination at all

  Parameters
	line_in:		Pointer pointing to the start of the string
	max:			Limit
					
  Return:
	value:		8bits value
-------------------------------------------------------------------------------
*/
BYTE data_strnlen(CHAR *p, BYTE max)
{
    BYTE b = 0;
	
	// While not ran out of string
	while((p[b] != 0) && (b < max)) {
		b++;
	}

    return b;
}

/*
-------------------------------------------------------------------------------
							data_strncpy
							============
  Abstract:

	copys the string length but takes a limit into account for whemn there is 
	no zero termination at all

  Parameters
  	line_out:		Pointer pointing to the start of receiving string
	line_in:		Pointer pointing to the start of the string
	max:			Limit
					
  Return:
	value:		8bits value
-------------------------------------------------------------------------------
*/
BYTE data_strncpy(CHAR *p1, CHAR *p2, BYTE max)
{
    BYTE b = 0;
	
	// While not ran out of string
	while((p2[b] != 0) && (b < max)) {
		p1[b] = p2[b];
		b++;
		if(b<max) p1[b] = 0; // terminate when fit
	}

    return b;
}

BYTE data_strnicpy(CHAR *p1, CHAR *p2, BYTE max) // copy to lowercase
{
    BYTE b = 0;
	
	// While not ran out of string
	while((p2[b] != 0) && (b < max)) {
        if((p2[b]>='A')&&(p2[b]<='Z'))
    	    p1[b] = p2[b] | 0x20;
    	else
    	    p1[b] = p2[b];

		b++;
		if(b<max) p1[b] = 0; // terminate when fit
	}

    return b;
}

#if 0
/*
-------------------------------------------------------------------------------
							data_itoa
							=========
  Abstract:
	
	converts value to ascii HEX string. (uses the default type)

  Parameters
	p:		Pointer pointing to the start of the buffer receiving the string
	i:		Integer to convert to ascii
	radix:	The radix of the integer (10 / 16)
					
  Return:
	value:	Pointer to the end of the conversion
-------------------------------------------------------------------------------
*/
CHAR *data_itoa(CHAR *p, unsigned i, unsigned radix)
{
    unsigned j;
    CHAR *q;

    q = p + MAXDIG;
    do {
        j = (unsigned) (i % radix);
        j += '0';
        if (j > '9') {
            j += 'A' - '0' - 10;
        }

        *--q = (CHAR) j;
        i = i / radix;
    } while(i);

    j = p + MAXDIG - q;
    do {
        *p++ = *q++;
    } while (--j);

	*p = 0; // NULL TERMINATE THE STRING
		
    return(p);
}
#endif

/*
-------------------------------------------------------------------------------
							data_itoa
							=========
  Abstract:
	
	converts value to ascii HEX string. (uses the long type)

  Parameters
	p:		Pointer pointing to the start of the buffer receiving the string
	i:		Integer to convert to ascii
	radix:	The radix of the integer (10 / 16)
					
  Return:
	value:	Pointer to the end of the conversion
-------------------------------------------------------------------------------
*/
CHAR *data_ltoa(CHAR *p, ULONG i, unsigned radix)
{
    ULONG j;
    CHAR *q;

    q = p + MAXDIG;
    do {
        j = (ULONG) (i % radix);
        j += '0';
        if (j > '9') {
            j += 'A' - '0' - 10;
        }

        *--q = (CHAR) j;
        i = i / radix;
    } while(i);

    j = p + MAXDIG - q;
    do {
        *p++ = *q++;
    } while (--j);

	*p = 0; // NULL TERMINATE THE STRING
		
    return(p);
}


