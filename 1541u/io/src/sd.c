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
 *	 Functions directly handling the SD-Card SPI interface.
 */
#pragma codeseg ("FILESYS")

#include "manifest.h"
#include <stdio.h>
#include "sd.h"
#include "spi.h"
#include "dump_hex.h"
#include "gpio.h"

#define TXT printf
#define DBG(x) x

#define READ_SPEED  0
#define WRITE_SPEED 0
//#define SD_VERIFY

BYTE sd_ver = 0;
BYTE sd_cap = 0;

/*
-------------------------------------------------------------------------------
							sd_Init
							=======
  Abstract:

	Intialized the SD-Card

  Return:
	0:		No errors
	other:	Function specific error
-------------------------------------------------------------------------------
*/
esint8 sd_Init(void)
{
	esint16 i;
	euint8 resp;
//	euint32 size;
//	char error;
	
    sd_cap = 0;
    sd_ver = 0;

	spi_SetSpeed(200); // slllooooowww

//    printf("Reset card...");
	/* Try to send reset command up to 100 times */
	i=100;
	do{
		sd_Command(0, 0, 0);
//		printf("%02x ", resp);
		resp=sd_Resp8b();
	}
	while(resp!=1 && i--);
	
    //printf("Tried %d times. %02x\n", 100-i, resp);

	if(resp!=1){
		if(resp==0xff){
			return(-1);
		}
		else{
			sd_Resp8bError(resp);
			return(-2);
		}
	}

    sd_Command(8, 0, 0x01AA);
    resp = sd_Resp8b();
//    printf("Response to cmd 8 = %02x.\n", resp);
    if(resp & 0x04) {
//        printf("SD V1.x\n");
        sd_ver = 1;
    } else {
//        printf("SD V2.00\n");
        resp = spi_Send(0xFF);
//        printf("Cmd Version: %02x ", resp);
        spi_Send(0xFF);
        resp = spi_Send(0xFF);
//        printf("Voltage: %02x\n", resp);
        resp = spi_Send(0xFF);
        if(resp != 0xAA) {
            return -4; // unusable card
        }
        sd_ver = 2;
    }
        
    //printf("Wait for card to initialize.\n\r");
	/* Wait till card is ready initialising (returns 0 on CMD1) */
	/* Try up to 32000 times. */
	i=32000;
	do{
//		sd_Command(1, 0, 0);
        sd_Command(CMDAPPCMD, 0, 0);
        resp=sd_Resp8b();
        //sd_Resp8bError(resp);
        sd_Command(41, 0x4000, 0);
		
		resp=sd_Resp8b();
//		printf("%02x ", resp);
		if((resp!=0)&&(resp!=1))
			sd_Resp8bError(resp);
	}
	while(resp==1 && i--);
	
    //printf("Waiting for %d. %02x\n", 32000-i, resp);

	if(resp!=0){
//		sd_Resp8bError(resp);
		return(-3);
	}

    if(sd_ver == 2) {
        sd_Command(CMDREADOCR, 0, 0);
        resp=sd_Resp8b();
        sd_Resp8bError(resp);
        resp = spi_Send(0xFF);
        spi_Send(0xFF);
        spi_Send(0xFF);
        spi_Send(0xFF);
        if(resp & 0x40) {
            sd_cap = 1;
//            printf("High capacity!\n");
        }
    }

	spi_SetSpeed(READ_SPEED); 

/*
    error = sd_getDriveSize(&size);
    printf("error=%d. Size = %lu.\n", error, size);
*/
/*
    printf("Set Block Len.\n");
    sd_Command(CMDSETBLKLEN, 0, 512);
	resp=sd_Resp8b();
    printf("error=%x\n", resp);
	sd_Resp8bError(resp);

    printf("Get status.\n");
	sd_Command(CMDGETSTATUS, 0, 0);
	resp=sd_Resp8b();
    printf("error=%x\n", resp);
	sd_Resp8bError(resp);
*/	
	return(0);
}

/*
-------------------------------------------------------------------------------
							sd_Resp8b
							=========
  Abstract:
  
	Gets an 8bits response from the SD-Card

  Return:
	resp:		the 8bits response
-------------------------------------------------------------------------------
*/
euint8 sd_Resp8b(void)
{
	euint8 i;
	euint8 resp;
	
//    printf("Resp:");
	/* Respone will come after 1 - 8 pings */
	for(i=0;i<8;i++){
		resp = spi_Send(0xff);
//        printf(" %02x", resp);
		if(resp != 0xff) {
//            printf("\n");
			return(resp);
	    }
	}
//    printf("\n");
	return(resp);
}

/*
-------------------------------------------------------------------------------
							sd_Resp16b
							==========
  Abstract:
  
	Gets an 16bits response from the SD-Card

  Return:
	resp:		the 16bits response
-------------------------------------------------------------------------------
*/
euint16 sd_Resp16b(void)
{
	euint16 resp;
	
	resp = ( ((euint16)sd_Resp8b()) << 8 ) & 0xff00;
	resp |= spi_Send(0xff);
	
	return(resp);
}

/*
-------------------------------------------------------------------------------
							sd_Resp8bError
							==============
  Abstract:
  
	Displays the error message that goes with the error
	FOR DEBUG PURPOSES
	
  Parameters
	value:		error code
-------------------------------------------------------------------------------
*/
void sd_Resp8bError(euint8 value)
{
    if(value == 0xFF) {
        DBG((TXT("Unexpected IDLE byte.\n")));
        return;
    }
    if(value & 0x40)  DBG((TXT("Argument out of bounds.\n")));
    if(value & 0x20)  DBG((TXT("Address out of bounds.\n")));
    if(value & 0x10)  DBG((TXT("Error during erase sequence.\n")));
    if(value & 0x08)  DBG((TXT("CRC failed.\n")));
    if(value & 0x04)  DBG((TXT("Illegal command.\n")));
	if(value & 0x02)  DBG((TXT("Erase reset (see SanDisk docs p5-13).\n")));
    if(value & 0x01)  DBG((TXT("Card is initialising.\n")));
}

/*
-------------------------------------------------------------------------------
							sd_State
							========
  Abstract:
  
	Gets the state of the SD-CARD

  Return:
	1:		normal state
	-1:		error state	
-------------------------------------------------------------------------------
*/
esint8 sd_State(void)
{
	eint16 value;
	
	sd_Command(13, 0, 0);
	value=sd_Resp16b();
	
//#if 0 // Implemented differently to save some bytes
	switch(value)
	{
		case 0x0000:
			DBG((TXT("Everything OK.\n")));
			return(1);
			break;
		case 0x0001:
			DBG((TXT("Card is Locked.\n")));
			break;
		case 0x0002:
			DBG((TXT("WP Erase Skip, Lock/Unlock Cmd Failed.\n")));
			break;
		case 0x0004:
			DBG((TXT("General / Unknown error -- card broken?.\n")));
			break;
		case 0x0008:
			DBG((TXT("Internal card controller error.\n")));
			break;
		case 0x0010:
			DBG((TXT("Card internal ECC was applied, but failed to correct the data.\n")));
			break;
		case 0x0020:
			DBG((TXT("Write protect violation.\n")));
			break;
		case 0x0040:
			DBG((TXT("An invalid selection, sectors for erase.\n")));
			break;
		case 0x0080:
			DBG((TXT("Out of Range, CSD_Overwrite.\n")));
			break;
		default:
			if(value>0x00FF)
				sd_Resp8bError((euint8) (value>>8));
			else
				printf("Unknown error: 0x%x (see SanDisk docs p5-14).\n",value);
			break;
	}
/*
#else
	if(value == 0x000)
		return(1);

	if(value > 0x00FF)
		sd_Resp8bError((euint8) (value>>8));

#endif
*/
	return(-1);
}

#ifdef SD_VERIFY
/*
-------------------------------------------------------------------------------
							sd_verifySector
-------------------------------------------------------------------------------
*/
esint8 sd_verifySector(euint32 address, euint8* buf)
{
    static euint8 rb_buffer[512];
    esint8 read_resp;
    euint16 i;
    
    read_resp = sd_readSector(address, &rb_buffer[0], 512);
    
    if(read_resp) {
        return read_resp;
    }
    for(i=0;i<512;i++) {
        if(rb_buffer[i] != buf[i]) {
            printf("VERIFY ERROR!\n");
            return -10;
        }
    }
    return 0;
}
#endif

/*
-------------------------------------------------------------------------------
							sd_writeSector
							==============
  Abstract:
  
	Writes a sector on the SD-Card

  Parameters
	address:	sector address to write to
	buf:		pointer to the buffer containing the data to write

  Return:
	0:		no error
	other:	error
-------------------------------------------------------------------------------
*/
/* ****************************************************************************
 * WAIT ?? -- FIXME
 * CMDWRITE
 * WAIT
 * CARD RESP
 * WAIT
 * DATA BLOCK OUT
 *      START BLOCK
 *      DATA
 *      CHKS (2B)
 * BUSY...
 */

esint8 sd_writeSector(euint32 address, euint8* buf)
{
	euint32 place;
	euint16 t=0; //,i;
	euint8  resp;
#ifdef SD_VERIFY
    esint8  ver;
#endif    
//    printf("Trying to write %p to sector %lu.\n",buf,address);
//    dump_hex(buf, 512);

	place=(sd_cap)?(address):(address<<9);

//	spi_SetSpeed(WRITE_SPEED); 

/*
    // wait a bit?
    for(t=0;t<16;t++) {
        resp=spi_Send(0xFF);
        printf(" %02x", resp);
    }   printf("\n");
*/

	sd_Command(CMDWRITE, (euint16) (place >> 16), (euint16) place);

    // wait for 0.5 ms
    TIMER = 100;
    while(TIMER)
        ;
    
	resp = sd_Resp8b(); /* Card response */
	sd_Resp8bError(resp);

    t = spi_WriteBlockFast(buf);

/*
    printf("nopped %u times.\n", t);

    if(t < 20)
        sd_State();
*/

#ifdef SD_VERIFY
    ver = sd_verifySector(address, buf);
    if(ver)
        return ver;
#endif

/*
    // Verify

	spi_Send(0xfe); // Start block 
	for(i=0;i<512;i++) 
		spi_Send(buf[i]); // Send data 
	spi_Send(0xff); // Checksum part 1 
	spi_Send(0xff); // Checksum part 2 
	spi_Send(0xff);

    while(spi_Send(0xff)!=0xff){
		t++;
		// Removed NOP 
	}
*/



/*
    if(t < 20) {
        printf("Probably not well written, but why? Let's try it slower..\n");
        
    	sd_Command(CMDWRITE, (euint16) (place >> 16), (euint16) place);
    	sd_Resp8b(); // Card response

    	spi_SetSpeed(WRITE_SPEED); 
        t = spi_WriteBlockFast(buf);
        
        printf("nopped %u times (slow nops).\n", t);
    }    	
*/
/*
//    t = spi_WriteBlockFast(buf);
    spi_WriteBlockFast(buf);
*/
	/*DBG((TXT("Nopp'ed %u times.\n"),t));*/

	return(0);
}

/*
-------------------------------------------------------------------------------
							sd_readSector
							==============
  Abstract:
  
	Reads a sector from the SD-Card

  Parameters
	address:	sector address to read from
	buf:		pointer to the buffer receiving the data to read
	len:		how much to read in bytes

  Return:
	0:		no error
	other:	error
-------------------------------------------------------------------------------
*/
/* ****************************************************************************
 * WAIT ?? -- FIXME
 * CMDCMD
 * WAIT
 * CARD RESP
 * WAIT
 * DATA BLOCK IN
 * 		START BLOCK
 * 		DATA
 * 		CHKS (2B)
 */

esint8 sd_readSector(euint32 address, euint8* buf, euint16 len)
{
	euint8 cardresp;
	euint8 firstblock;
//	euint8 c;
	euint16 fb_timeout=0x1fff;
//	euint32 i;
	euint32 place;

    len = len;
//	DBG((TXT("sd_readSector::Trying to read sector %u and store it at %p.\n"),address,&buf[0]));
//    printf("Trying to read sector %lu to %p.\n",address,buf);

//	spi_SetSpeed(READ_SPEED); 

/*
    // wait a bit?
    for(c=0;c<16;c++) {
        cardresp=spi_Send(0xFF);
        printf(" %02x", cardresp);
    }   printf("\n");
*/
	place=(sd_cap)?(address):(address<<9);

	sd_Command(CMDREAD, (euint16) (place >> 16), (euint16) place);
	
	cardresp=sd_Resp8b(); /* Card response */ 

	/* Wait for startblock */
	do
		firstblock=sd_Resp8b(); 
	while(firstblock==0xff && fb_timeout--);

	if(cardresp!=0x00 || firstblock!=0xfe){
		sd_Resp8bError(firstblock);
		return(-1);
	}
/*	printf("sd_readSector command seems to be ok..\n"); */
	
/*
    if(len < 512) {
    	for(i=0;i<512;i++){
    		c = spi_Send(0xff);
    //		if(i<16)
    //			printf("%02X ", c);
    		if(i<len)
    			buf[i] = c;
    	}
    //	printf("\n");
    } else {
*/
        spi_ReadBlockFast(buf);
//	}
	
	/* Checksum (2 byte) - ignore for now */
	spi_Send(0xff);
	spi_Send(0xff);

//    dump_hex(buf, 512);

	return(0);
}
/*
-------------------------------------------------------------------------------
							sd_getDriveSize
							===============
  Abstract:
  
	Calculates size of card from CSD 

  Parameters
	drive_size:	pointer to a 32bits variable receiving the size

  Return:
	0:		no error
	other:	error
-------------------------------------------------------------------------------
*/
//#define SD_DRIVE_SIZE
#ifdef SD_DRIVE_SIZE

esint8 sd_getDriveSize(euint32* drive_size )
{
	euint8 cardresp, i, by, q;
	euint8 iob[16];
	euint16 c_size, c_size_mult, read_bl_len;
	
	sd_Command(CMDREADCID, 0, 0);
	
    q = 200;
	do {
		cardresp = sd_Resp8b();
		q--;
	} while ((cardresp != 0xFE) && (q));

    if(q) {
        printf("CID:");
    	for( i=0; i<16; i++) {
    		iob[i] = spi_Send(0xFF);
    		printf(" %02x", iob[i]);
    	}
    	printf("\n");
    } else {
        printf("Failed to read Card ID.\n");
    }
	sd_Command(CMDREADCSD, 0, 0);
	
    q = 200;
	do {
		cardresp = sd_Resp8b();
		q--;
	} while ((cardresp != 0xFE) && (q));

    if(q) {
        printf("CSD:");
    	for( i=0; i<16; i++) {
    		iob[i] = spi_Send(0xFF);
    		printf(" %02x", iob[i]);
    	}
    	printf("\n");
    } else {
        printf("Failed to read CSD.\n");
    	spi_Send(0xff);
    	spi_Send(0xff);
        *drive_size = 0L;
        return 1;
    }

	spi_Send(0xff);
	spi_Send(0xff);
	
	c_size = iob[6] & 0x03; // bits 1..0
	c_size <<= 10;
	c_size += (euint16)iob[7]<<2;
	c_size += iob[8]>>6;

	by= iob[5] & 0x0F;
	read_bl_len = 1;
	read_bl_len <<= by;

	by=iob[9] & 0x03;
	by <<= 1;
	by += iob[10] >> 7;
	
	c_size_mult = 1;
	c_size_mult <<= (2+by);
	
//    printf("read_bl_len = %d. size_mult = %d. c_size = %d.\n", read_bl_len, c_size_mult, c_size);

	*drive_size = (euint32)(c_size+1) * (euint32)c_size_mult * (euint32)read_bl_len;
	
	return 0;
}
#endif
