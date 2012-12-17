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
 *	 Functions for handling atmel flash memory
 *
 */
	
#include "manifest.h"
#include <stdio.h>
#include "mapper.h"
#include "ff.h"
#include "flash.h"
#include "gpio.h"

#define FLASH_CMD1  *((BYTE *)(MAP1_ADDR+0x0AAA))
#define FLASH_CMD2  *((BYTE *)(MAP1_ADDR+0x1554))
#define FLASH_MANID *((BYTE *)(MAP1_ADDR+0x0000))
#define FLASH_DEVID *((BYTE *)(MAP1_ADDR+0x0002))

BYTE flash_id[2];

WORD flash_get_cfg_page(void)
{
    return MAP_FLASH; // Bottom page!
}

WORD flash_get_cfg_sector(void)
{
    return 0;
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
							flash_get_id
							============
  Abstract:

	Gets the Manufacturer and Device Code

  Parameters
	none
	
-------------------------------------------------------------------------------
*/
void flash_get_id(void)
{
    MAPPER_MAP1 = MAP_FLASH;
    
    FLASH_CMD1 = 0xAA;
    FLASH_CMD2 = 0x55;
    FLASH_CMD1 = 0x90;
    flash_id[0] = FLASH_MANID;
    flash_id[1] = FLASH_DEVID;
    FLASH_CMD1 = 0xAA;
    FLASH_CMD2 = 0x55;
    FLASH_CMD1 = 0xF0;
}


/*
-------------------------------------------------------------------------------
							flash_addr2block
							================
  Abstract:

	Calculates the block number belonging to an address

  Parameters
	addr:		address
	
-------------------------------------------------------------------------------
*/
WORD flash_addr2block(LONG addr)
{
    WORD d = (addr >> 13); // convert to 8K block index first
/* TOP BOOT */
/*
    if(d < 248)
        return (d >> 3);
    return d - 217;
*/
/* BOTTOM BOOT */
    if(d < 8)
        return d;
    return (d >> 3) + 7;
}

/*
-------------------------------------------------------------------------------
							flash_erase_block
							=================
  Abstract:

	Erases a flash block

  Parameters
	d:		block number

  Return value:
    BOOL    true = success, false = error	
-------------------------------------------------------------------------------
*/
BOOL flash_erase_block(WORD d)
{
    WORD tout;
    
/* TOP BOOT */
/*
    if(d < 31)
        MAPPER_MAP1 = (d << 3) + MAP_FLASH; // Mapper blocks are 8K, device blocks are 64K
    else
        MAPPER_MAP1 = d + MAP_FLASH + 0xD9;
*/
/* BOTTOM BOOT */
    if(d < 8)
        MAPPER_MAP1 = d + MAP_FLASH;
    else
        MAPPER_MAP1 = ((d-7) << 3) + MAP_FLASH;

//    printf("Erasing block %d...", d); //, MAPPER_MAP1);

	// erase block in flash
    FLASH_CMD1 = 0xAA;
    FLASH_CMD2 = 0x55;
    FLASH_CMD1 = 0x80;
    FLASH_CMD1 = 0xAA;
    FLASH_CMD2 = 0x55;
    FLASH_CMD2 = 0x30;

	// check status
    TIMER = 200;
    tout  = 2000;
	while(!(FLASH_CMD1 & 0x80)) {
        if(!TIMER) {
            TIMER = 200;
            tout--;
            if(!tout)
                return FALSE;
        }
    }
    return TRUE;
}

/*
-------------------------------------------------------------------------------
							flash_erase
							===========
  Abstract:

	Erases the complete flash
	
-------------------------------------------------------------------------------
*/
BOOL flash_erase(void)
{
    WORD tout;
    
    MAPPER_MAP1 = MAP_FLASH;

	// erase chip
    FLASH_CMD1 = 0xAA;
    FLASH_CMD2 = 0x55;
    FLASH_CMD1 = 0x80;
    FLASH_CMD1 = 0xAA;
    FLASH_CMD2 = 0x55;
    FLASH_CMD1 = 0x10;

	// check status
	// check status
    TIMER = 200;
    tout  = 15000;
	while(!(FLASH_CMD1 & 0x80)) {
        if(!TIMER) {
            TIMER = 200;
            tout--;
            if(!tout)
                return FALSE;
        }
    }
    return TRUE;
}

/*
-------------------------------------------------------------------------------
							flash_protect
							=============
  Abstract:

	Protects the upper 192K for the BOOT FPGA
	
-------------------------------------------------------------------------------
*/
void flash_protect(void)
{
    WORD d;
    for(d = MAP_FLASH + 0xE8; d < MAP_FLASH + 0x100; d+= 8) {
        MAPPER_MAP1 = d;

    	// sector lockdown
        FLASH_CMD1 = 0xAA;
        FLASH_CMD2 = 0x55;
        FLASH_CMD1 = 0x80;
        FLASH_CMD1 = 0xAA;
        FLASH_CMD2 = 0x55;
        FLASH_CMD1 = 0x60;

        TIMER = 40;
        while(TIMER)
            ;
    }
    printf("Boot area of Flash protected.\n");
}

/*
-------------------------------------------------------------------------------
							flash_file
							==========
  Abstract:

	Programs an intel hex file to the flash
	
-------------------------------------------------------------------------------
*/
BOOL flash_file(FIL *fp, BYTE v)
{
	static WORD  bytes_read;
	static CHAR  buf[8];
	static BYTE	 chk;
    static WORD  d, last_erased = 0xFFFF;
	static BYTE  data[64], i, b, r;
	static LONG  addr;
	static BYTE  *paddr;
	static BYTE  *flash;
	static SHORT mapped;
    static BYTE  verify;
    static BYTE  errors;
    static WORD  timeout;
        
    errors = 0;
    verify = v;
	paddr = (BYTE *)&addr;
	flash = (BYTE *)MAP1_ADDR;

    MAPPER_MAP1 = MAP_FLASH; // 0x800 x 8k = 16M (start of flash)

    flash_get_id();
    printf("Flash ID: %02X %02X.\n", flash_id[0], flash_id[1]);

	while(errors < 40)
	{
		// find start of record
		do {
		    f_read(fp, buf, 1, &bytes_read);
		} while(bytes_read && (buf[0] != ':'));
		if(!bytes_read)
			return TRUE;

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
				return TRUE;

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

                d = flash_addr2block(addr);

                if(!verify) {
				// check if block has already been erased
    				if 	(last_erased != d) {
                        if(!flash_erase_block(d))
                            return FALSE;
    					last_erased = d;
                    }
                }

				d = (WORD)(addr >> 13); // calculate map
				MAPPER_MAP1 = (d + MAP_FLASH);
                mapped = (SHORT)(addr & 0x1FFF);
//				printf("%d bytes of data to address %08lx. Map = %04x\n", (SHORT)data[0], addr, MAPPER_MAP1);

				for(i=0;i<data[0];i++) {
                    // program byte
                    b = data[4+i];
                    if(!verify) {
                        FLASH_CMD1 = 0xAA;
                        FLASH_CMD2 = 0x55;
                        FLASH_CMD1 = 0xA0;
                        flash[mapped] = b;
                        b &= 0x80;
                        timeout = 0x7FFF;
                        while(--timeout) {
                            r = flash[mapped];
                            if((r & 0x80)==b)
                                break;
                            if((r & 0x20)==0x20) {
                                r = flash[mapped];
                                if((r & 0x80)==b)
                                    break;
                                printf("error.");
                                FLASH_CMD1 = 0xF0;
                                return FALSE;
                            }                                
                        }
                        if(!timeout) {
                            printf("timeout. MAPPER=%04x, mapped=%04x, data=%02x %02x\n", MAPPER_MAP1, mapped, b, r);
                            return FALSE;
                        }
                    } else {
                        if(flash[mapped] != b) {
                            printf("Verify error at block %d, offset %04x. Read %02x, should be %02x\n",
                                    d, mapped, flash[mapped], b);
                            errors++;
                        }
                    }
                    mapped++;
                }
				break;

			default:
				printf("Unsupported record type %2x.\n", data[3]);
		}
	}
	printf("Too many errors.. exiting.\n");
	return FALSE;
}


