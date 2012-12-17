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
 */

#ifndef __EFS_TYPES_H__
#define __EFS_TYPES_H__

typedef unsigned char	BYTE;
typedef unsigned char	UCHAR;
typedef char			CHAR;
typedef unsigned short	WORD;
typedef unsigned short	USHORT;
typedef short			SHORT;
typedef unsigned long	DWORD;
typedef unsigned long	ULONG;
typedef long			LONG;

typedef unsigned char	BOOL;

typedef char 		   eint8;
typedef signed char    esint8;
typedef unsigned char  euint8;
typedef short          eint16;
typedef signed short   esint16;
typedef unsigned short euint16; 
typedef long           eint32; 
typedef signed long    esint32;
typedef unsigned long  euint32; 

#define FALSE	0
#define TRUE	1

#define NR_OF_EL(a)		(sizeof(a) / sizeof(a[0]))
#define UNREFERENCED_PAR(a)	a=a

#endif
