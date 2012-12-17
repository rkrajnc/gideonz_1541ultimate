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
 *    Abstract:
 *
 *	API to the SD-Card
 */

#pragma codeseg ("FILESYS")

#include "manifest.h"
#include <stdio.h>
#include "diskio.h"
#include "sd.h"
#include "spi.h"


static DSTATUS status;

/*
-------------------------------------------------------------------------------
							<function name>
							===============
  Abstract:

	<insert UNDERSTANDABLE description here>

  Parameters
	<parameter>:		<insert UNDERSTANDABLE description here>
	

  Return:
	<insert value>:		<insert UNDERSTANDABLE description here>
	other:				<insert UNDERSTANDABLE description here>
-------------------------------------------------------------------------------
*/
DSTATUS disk_initialize()
{
	esint8 res;
//    esint8 res2;
//  euint16 errors;
//    euint32 i;
    
//    static euint8 test_buffer[512];

	spi_Init();
	res = sd_Init();
    	
    // Test SD card writes
//    for(i=0;i<512;i++) {
//        test_buffer[i] = (i<32)?0x47:0;
//   }

/*
    sd_readSector(2083, test_buffer, 512);
    test_buffer[0] = 'G';

    errors = 0;
    for(i=2083L;i<2500L;i++) {
        res2 = sd_writeSector(i, test_buffer);
        if(res2) {
            errors++;
            printf("E%d",res2);
        } else {
            printf(".");
        }
    }
*/

	status = 0;
	if(res < 0) {
		status = STA_NOINIT;
	}
	return status;
}

/*
-------------------------------------------------------------------------------
							disk_read
							=========
  Abstract:

	Reads from the SD-Card

  Parameters
	buffer:		pointer to a buffer receiving the read data
	sector:		sector to read from
	numsect:	number of sectors to read
	
  Return:
	result:		RES_OK or RES_ERROR
-------------------------------------------------------------------------------
*/
DRESULT disk_read (BYTE *buffer, DWORD sector, BYTE numsect)
{
	BYTE s;
	
//	printf("Disk Read. Sector = %ld, num = %d, buffer = %x\n", sector, numsect, buffer);

	for(s=0;s<numsect;s++) {
		if(sd_readSector(sector, buffer, 512)) {
            printf("Disk read error.\n");
			return RES_ERROR;
		}
		buffer += 512;
		++sector;
	}
	return RES_OK;
}

/*
-------------------------------------------------------------------------------
							disk_write
							==========
  Abstract:

	Reads from the SD-Card

  Parameters
	buffer:		pointer to a buffer containg the data to write
	sector:		sector to read from
	numsect:	number of sectors to read
	
  Return:
	result:		RES_OK or RES_ERROR
-------------------------------------------------------------------------------
*/
DRESULT disk_write (const BYTE *buffer, DWORD sector, BYTE numsect)
{
	BYTE s;
	
//	printf("Disk Write. Sector = %ld, num = %d.\n", sector, numsect);

	for(s=0;s<numsect;s++) {
		if(sd_writeSector(sector, (euint8*)buffer)) {
            printf("Verify ERROR.\n");
			return RES_ERROR;
		}
		buffer += 512;
		++sector;
	}
//    printf("Disk write ok.\n");
	return RES_OK;
}


/*
-------------------------------------------------------------------------------
							<function name>
							===============
  Abstract:

	Yet to implement functions

  Parameters
	<parameter>:		<insert UNDERSTANDABLE description here>
	

  Return:
	<insert value>:		<insert UNDERSTANDABLE description here>
	other:				<insert UNDERSTANDABLE description here>
-------------------------------------------------------------------------------
*/
DSTATUS disk_shutdown ()
{
	// do nothing
	return 0;
}

DSTATUS disk_status ()
{
	// we should check the protect and card detect bits
	return status;
}


DRESULT disk_ioctl (BYTE, void *)
{
	return RES_OK;
}

void disk_timerproc ()
{
}

DWORD get_fattime(void)
{
	/*
bit31:25 
Year from 1980 (0..127) 
bit24:21 
Month (1..12) 
bit20:16 
Date (1..31) 
bit15:11 
Hour (0..23) 
bit10:5 
Minute (0..59) 
bit4:0 
Second/2 (0..29) 
*/
	/* 0011010 1100 11001 10010 100101 00000   Dec 25, 2006   18:35:00 */
	/* 0011 0101 1001 1001 1001 0100 1010 0000 */

	return 0x359994A0;
}


