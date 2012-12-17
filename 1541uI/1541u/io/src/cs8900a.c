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
 *	 Functions for initializing and using the Ethernet chip
 */
#include <stdio.h>
#include "types.h"
#include "mapper.h"
#include "gpio.h"

#include "cs8900a.h"

// Offset Type Description
// 0000h Read/Write Receive/Transmit Data (Port 0)
// 0002h Read/Write Receive/Transmit Data (Port 1)
// 0004h Write-only TxCMD (Transmit Command)
// 0006h Write-only TxLength (Transmit Length)
// 0008h Read-only Interrupt Status Queue
// 000Ah Read/Write PacketPage Pointer
// 000Ch Read/Write PacketPage Data (Port 0)
// 000Eh Read/Write PacketPage Data (Port 1)

#define CS8900A_PAGE 0x0A00

#define ETH_XFER_DATA *((DWORD *)(MAP2_ADDR + 0x00))
#define ETH_XFER_DATA0 *((WORD *)(MAP2_ADDR + 0x00))
#define ETH_XFER_DATA1 *((WORD *)(MAP2_ADDR + 0x02))
#define ETH_TXCMD      *((WORD *)(MAP2_ADDR + 0x04))
#define ETH_TXLEN      *((WORD *)(MAP2_ADDR + 0x06))
#define ETH_INT_STAT   *((WORD *)(MAP2_ADDR + 0x08))
#define ETH_PP_PTR     *((WORD *)(MAP2_ADDR + 0x0A))
#define ETH_PP_DATA   *((DWORD *)(MAP2_ADDR + 0x0C))
#define ETH_PP_DATA0   *((WORD *)(MAP2_ADDR + 0x0C))
#define ETH_PP_DATA1   *((WORD *)(MAP2_ADDR + 0x0E))

#define CS8900A_AUTO_INC      0x8000
#define CS8900A_PROD_IDENT    0x3000
#define CS8900A_RXCFG         0x3102
#define CS8900A_RXCTL         0x3104
#define CS8900A_LINECTL       0x3112
#define CS8900A_SELFCTL       0x3114
#define CS8900A_LINE_ST       0x3134
#define CS8900A_BUS_ST        0x3138
#define CS8900A_IEEE_ADDR     0x3158

#define LINESTAT_LinkOK       0x0080
#define LINESTAT_PolarityOK   0x1000

#define BUSST_Rdy4TxNOW       0x0100
#define BUSST_TxBidErr        0x0080

#define RXCFG_ID              0x03
#define RXCFG_ExtradataiE     0x4000
#define RXCFG_RuntiE          0x2000
#define RXCFG_CRCerroriE      0x1000
#define RXCFG_BufferCRC       0x0800
#define RXCFG_AutoRxDMAE      0x0400
#define RXCFG_RxDMAonly       0x0200
#define RXCFG_RxOKiE          0x0100
#define RXCFG_StreamE         0x0080
#define RXCFG_Skip_1          0x0040

#define RXCTL_ID              0x05
#define RXCTL_IAHashA         0x0040
#define RXCTL_PromiscuousA    0x0080
#define RXCTL_ExtradataA      0x4000
#define RXCTL_RuntA           0x2000
#define RXCTL_CRCErrorA       0x1000
#define RXCTL_BroadcastA      0x0800
#define RXCTL_IndividualA     0x0400
#define RXCTL_MulticastA      0x0200
#define RXCTL_RxOKA           0x0100

#define LINECTL_ID            0x13
#define LINECTL_LoRxSquelch   0x4000
#define LINECTL_TwoPartDefDis 0x2000
#define LINECTL_PolarityDis   0x1000
#define LINECTL_ModBackoffE   0x0800
#define LINECTL_AutoAUI_10BT  0x0200
#define LINECTL_AUIonly       0x0100
#define LINECTL_SerTxON       0x0080
#define LINECTL_SerRxON       0x0040

#define SELFCTL_ID            0x15
#define SELFCTL_HCB1          0x8000
#define SELFCTL_HCB0          0x4000
#define SELFCTL_HC1E          0x2000
#define SELFCTL_HC0E          0x1000
#define SELFCTL_HWStandbyE    0x0400
#define SELFCTL_HWSleepE      0x0200
#define SELFCTL_SWSuspend     0x0100
#define SELFCTL_RESET         0x0040

#define RXEVENT_Extradata     0x4000
#define RXEVENT_Runt          0x2000
#define RXEVENT_CRCerror      0x1000
#define RXEVENT_Broadcast     0x0800
#define RXEVENT_Individual    0x0400
#define RXEVENT_AdrHashed     0x0200
#define RXEVENT_RxOK          0x0100
#define RXEVENT_Dribblebits   0x0080
#define RXEVENT_IAHash        0x0040


BYTE cs8900a_detect(void)
{
    WORD eisa_code;
    WORD prod_code;
    
    MAPPER_MAP2 = CS8900A_PAGE;

    ETH_PP_PTR = CS8900A_PROD_IDENT | CS8900A_AUTO_INC;
    
    eisa_code = ETH_PP_DATA0;
    prod_code = ETH_PP_DATA0;        

    printf("32 bit ident code: %04x-%04x\n", eisa_code, prod_code);
    
    if(eisa_code == 0x630E) {
        return 1;
    }
    return 0;
}


void cs8900a_init(void)
{
    WORD linestat;
    
    ETH_PP_PTR   = CS8900A_LINE_ST;
    linestat     = ETH_PP_DATA0;
    printf("Line Status: %04x ", linestat);
    if(linestat & LINESTAT_LinkOK) {
        printf("Link up\n");
    } else {
        printf("Link down\n");
    }

/*
    // reset the whole damn thing
    ETH_PP_PTR   = CS8900A_SELFCTL;
    ETH_PP_DATA0 = SELFCTL_ID | 
                   SELFCTL_RESET;
*/
    // set mac address
    ETH_PP_PTR   = CS8900A_IEEE_ADDR | CS8900A_AUTO_INC;
    ETH_PP_DATA0 = 0x1200; // octet 1/0
    ETH_PP_DATA0 = 0x5634; // octet 3/2
    ETH_PP_DATA0 = 0x9A78; // octet 5/4

    // initialize transmission
    // (no initialization necessary)
    
    // initialize reception
    ETH_PP_PTR   = CS8900A_RXCTL;
    ETH_PP_DATA0 = RXCTL_ID | 
                   RXCTL_RxOKA |
                   RXCTL_IndividualA |
                   RXCTL_BroadcastA |
                   RXCTL_CRCErrorA;
    
    ETH_PP_PTR   = CS8900A_RXCFG;
    ETH_PP_DATA0 = RXCFG_ID | 
                   RXCFG_RxOKiE;
                   
    // Initialize Line
    ETH_PP_PTR   = CS8900A_LINECTL;
    ETH_PP_DATA0 = LINECTL_ID | 
                   LINECTL_LoRxSquelch |
                   LINECTL_SerTxON |
                   LINECTL_SerRxON;

    do {
        ETH_PP_PTR   = CS8900A_LINE_ST;
        linestat     = ETH_PP_DATA0;
        printf("Line Status: %04x ", linestat);
        if(linestat & LINESTAT_LinkOK) {
            printf("Link up\n");
        } else {
            printf("Link down\n");
        }
    } while(!(linestat & LINESTAT_LinkOK));
    
    TIMER = 255;
    while(TIMER)
        ;

}


BYTE cs8900a_tx_frame(void *buffer, int length)
{
    WORD busst;
    WORD *data;
    
    // bid on tx buffer space
    ETH_TXCMD = 0x00C9; // standard frame transmit
    ETH_TXLEN = (WORD)length;
    
    // check on status
    ETH_PP_PTR = CS8900A_BUS_ST;
    busst = ETH_PP_DATA0;
    
    if(busst & BUSST_TxBidErr)
        return 0;
        
    // wait loop
    while(!(busst & BUSST_Rdy4TxNOW))
        busst = ETH_PP_DATA0;

    // copy frame
    data = (WORD *)buffer;
    while(length > 0) {
        ETH_XFER_DATA0 = *(data++);
        length -= 2;
    }
    // c8900a automatically transmits packet
}

int cs8900a_rx_frame(void *buffer)
{
    WORD intstat;
    WORD stat;
    int  len, remain;
    WORD *data;
    BYTE b;
    
    intstat = ETH_INT_STAT;
    
    if(!intstat) // no status
        return 0;
        
    if((intstat & 0x3F)!=0x04) {
        printf("CS8900A: Unknown event type: %04x\n", intstat);
        return 0;
    } else {
        printf("CS8900A: RxEvent: %04x\n", intstat);
        if((intstat & RXEVENT_RxOK)||(intstat & RXEVENT_CRCerror)) {
            stat = ETH_XFER_DATA0;
            len  = (int)ETH_XFER_DATA0;
            data = (WORD *)buffer;
            if(len>1536) {
                printf("Bad length: %d\n", len);
                return 0;
            }
            printf("Length = %d, status: %04x\n", len, stat);
            remain = len;
            
            b = *((BYTE *)(MAP2_ADDR + 0x01));
            while(remain > 0) {
                *(data++) = ETH_XFER_DATA0;
                remain -= 2;
            }

/*
            ETH_PP_PTR = 0xB404;
            while(remain > 0) {
                *(data++) = ETH_PP_DATA0;
                remain -= 2;
            }
*/
            return len;
        }
    }

    return 0; // some other event we didn't understand or ask for
}
