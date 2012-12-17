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
 *	 Exported items allowing use of the SD-Card interface.
 */
#ifndef __SD_H_ 
#define __SD_H_ 

#include "types.h"


#define CMDGETSTATUS 13
#define CMDSETBLKLEN 16
#define	CMDREAD      17
#define	CMDWRITE     24
#define	CMDREADCSD    9
#define	CMDREADCID   10
#define CMDCRCONOFF  59
#define CMDAPPCMD    55
#define CMDREADOCR   58

esint8  sd_Init(void);
void    sd_Command(euint8 cmd, euint16 paramx, euint16 paramy);
euint8  sd_Resp8b(void);
void    sd_Resp8bError(euint8 value);
euint16 sd_Resp16b(void);
esint8  sd_State(void);

esint8 sd_readSector(euint32 address,euint8* buf, euint16 len);
esint8 sd_writeSector(euint32 address, euint8* buf);
esint8 sd_getDriveSize( euint32* drive_size );

/* DIRTY. The following declaration is for calling this function from bank 0 */
esint8 bank1_sd_writeSector(euint32 address, euint8* buf);

void check_sd(void);

extern BYTE card_change;
extern BYTE card_det;

#endif
