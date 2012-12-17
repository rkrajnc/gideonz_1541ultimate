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
 *	 Functions to handle Intel Flash memory (obsolete)
 */
#include "manifest.h"
#include <stdio.h>
#include "mapper.h"
#include "ff.h"
#include "flash.h"
#include "console.h"

#define FLASH_CMD  *((BYTE *)0x4000)


WORD flash_get_cfg_page(void)
{
    return MAP_FLASH + 12; // 384K above base.
}

WORD flash_get_cfg_sector(void)
{
    return 3; // 384K above base.
}

/*
-------------------------------------------------------------------------------
							read_hexbyte
							============
  Abstract:

	Functions retrieves a byte from an intel hexfile
	
  Parameters
	
  Return:
	byte:		the retrieved byte
-------------------------------------------------------------------------------
*/
BYTE read_hexbyte(FIL *fp)
{
	static CHAR buf[2];
	static WORD bytes_read;
	BYTE res = 0;
    f_read(fp, buf, 2, &bytes_read);
	if(bytes_read == 2) {
		if((buf[0] >= '0')&&(buf[0] <= '9')) res = (buf[0] & 0x0F) << 4;
		else if((buf[0] >= 'A')&&(buf[0] <= 'F')) res = (buf[0] - 55) << 4;
		if((buf[1] >= '0')&&(buf[1] <= '9')) res |= (buf[1] & 0x0F);
		else if((buf[1] >= 'A')&&(buf[1] <= 'F')) res |= (buf[1] - 55);
	}
	return res;
}

/*
-------------------------------------------------------------------------------
							flash_erase_block
							=================
  Abstract:

	Erases a flash block

  Parameters
	d:		block number
	
-------------------------------------------------------------------------------
*/
void flash_erase_block(WORD d)
{
    printf("Erasing block %d... ", d);

    MAPPER_MAP1 = (d << 4) + 0x0800;

	// erase block in flash
	FLASH_CMD = 0x20;
	FLASH_CMD = 0xD0;

	// check status
	FLASH_CMD = 0x70;
	while(!(FLASH_CMD & 0x80))
		;
    if(FLASH_CMD = 0x80) {
		printf("done.\n");
	} else {
	    printf("error (%02X)\n", FLASH_CMD);
    }
}

/*
-------------------------------------------------------------------------------
							flash_erase
							===========
  Abstract:

	Erases the complete flash
	
-------------------------------------------------------------------------------
*/
void flash_erase(void)
{
    BYTE d;

    for(d=0;d<32;d++) {
        flash_erase_block((WORD)d);
    }
}

/*
-------------------------------------------------------------------------------
							flash_file
							==========
  Abstract:

	Programs an intel hex file to the flash
	
-------------------------------------------------------------------------------
*/
void flash_file(FIL *fp, BYTE v)
{
	static WORD  bytes_read;
	static CHAR  buf[8];
	static BYTE  chk;
    static WORD  d, last_erased = 0xFFFF;
	static BYTE  data[64], i, b;
	static LONG	 addr;
	static BYTE  *paddr;
	static BYTE  *flash;
	static SHORT mapped;
    static BYTE  verify;
    static BYTE  errors;
        
    errors = 0;
	paddr = (BYTE *)&addr;
	flash = (BYTE *)MAP1_ADDR;

    MAPPER_MAP1 = 0x0800; // 0x800 x 8k = 16M (start of flash)
    verify = v;
    
	printf("Flash file called.\n");
	FLASH_CMD = 0x90;
    printf("Flash ID: %02X %02X.\n", flash[0], flash[2]);
    FLASH_CMD = 0xFF;

	while(errors < 40)
	{
		// find start of record
		do {
		    f_read(fp, buf, 1, &bytes_read);
		} while(bytes_read && (buf[0] != ':'));
		if(!bytes_read)
			return;

		i=0;
		chk = 0;
//        printf("Record: ");
		do {
			d = read_hexbyte(fp);
			chk += d;
			data[i++] = d;
//            printf("%02X ", d);
		} while(i < (data[0] + 5));
//        printf("\n");

		if(chk) {
			printf("Checksum error.\n");
		}
		switch(data[3]) { // type field
			case 0x01:
				printf("End record.\n");
				return;

			case 0x02: // x86 extended memory format
				printf("Unsupported segmented address.\n");
				break; // ignored

			case 0x04: // x386 extended address format
				if(data[0] == 2) {
					paddr[3] = data[4];
					paddr[2] = data[5];
				} else {
					printf("Extended address with wrong length.\n");
				}
                break;

			case 0x00: // data
				paddr[0] = data[2];
				paddr[1] = data[1];
//				printf("%d bytes of data to address %08lx.\n", (SHORT)data[0], addr);

				// check if block has already been erased
				d = (WORD)(addr >> 17);
                if (!verify) {
    				if 	(last_erased != d) {
                        flash_erase_block(d);
    					last_erased = d;
                    }
                }

				d = (WORD)(addr >> 13); // calculate map
				MAPPER_MAP1 = (d + 0x0800);
                mapped = (SHORT)(addr & 0x1FFF);

				for(i=0;i<data[0];i++) {
                    b = data[4+i];
                    if(!verify) {
                        // program byte
                        FLASH_CMD = 0x40;
                        flash[mapped] = b;
    					while(!(FLASH_CMD & 0x80))
    						;
                    } else {
                        if(flash[mapped] != b) {
                            printf("Verify error at block %d, offset %04x. Read %02x, should be %02x\n",
                                    d, mapped, flash[mapped], b);
                            errors++;
                        }
                    }

				    mapped++;
                }
                FLASH_CMD = 0xFF; // switch to read mode
				break;

			default:
				printf("Unsupported record type %2x.\n", data[3]);
		}
	}
	printf("Too many errors.. exiting.\n");
}


