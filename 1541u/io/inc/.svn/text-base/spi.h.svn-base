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
 *	 Exported items allowing direct control of the SPI interface.
 */
#ifndef _SPI_H
#define _SPI_H

#include "types.h"

/*void sd_Command(euint8 cmd, euint16 paramx, euint16 paramy);*/
void    spi_Init(void);
void    spi_Clocks(euint8 len);
euint8  spi_Send(euint8 data);
void    spi_SetSpeed(euint8 rate);
void    spi_ReadBlockFast(euint8 *buf);
euint16 spi_WriteBlockFast(euint8 *buf);
euint8  CalcBlockChecksum(euint8 *buf);


#endif