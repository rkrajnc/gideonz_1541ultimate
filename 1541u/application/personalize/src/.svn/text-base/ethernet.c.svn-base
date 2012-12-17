#include <stdio.h>
#include "ethernet.h"
#include "cs8900a.h"
#include "dump_hex.h"
#include "gpio.h"

BYTE tx_buffer[1200];
BYTE rx_buffer[1200];

const BYTE mac[6] = { 0x00, 0x12, 0x34, 0x56, 0x78, 0x9a };
const BYTE src_ip[4]  = { 192, 168, 7, 64 };
const BYTE dst_ip[4]  = { 192, 168, 7, 65 };
    
void eth_header(BYTE *buf, WORD type) // makes IP header for broadcast
{
    // dst
    buf[0] = 0xFF;
    buf[1] = 0xFF;
    buf[2] = 0xFF;
    buf[3] = 0xFF;
    buf[4] = 0xFF;
    buf[5] = 0xFF;

    // src
    buf[6]  = mac[0];
    buf[7]  = mac[1];
    buf[8]  = mac[2];
    buf[9]  = mac[3];
    buf[10] = mac[4];
    buf[11] = mac[5];

    // ethernet type
    buf[12] = type >> 8;
    buf[13] = (BYTE)type;
}

void ip_header(BYTE *buf, WORD len, BYTE prot, DWORD src, DWORD dst)
{
    static WORD id = 0x1234;
    DWORD chk;
    BYTE b, *p;
        
    // header info
    buf[0] = 0x45; // length = 5, version = 4;
    buf[1] = 0x00; // TOS = normal packet
    len += 20;
    buf[2] = len >> 8;  // length high
    buf[3] = (BYTE)len; // length low
    buf[4] = id >> 8;   // id high
    buf[5] = (BYTE)id;  // id low
    buf[6] = 0x40;      // flags high (DF bit set)
    buf[7] = 0x00;      // flags low
    buf[8] = 0x10;      // max 16 hops
    buf[9] = prot;      // protocol
    buf[10] = 0;        // initial checksum
    buf[11] = 0;        // initial checksum

    id++;

    // IP addresses
    p = (BYTE *)&src;
    buf[12]= p[3];
    buf[13]= p[2];
    buf[14]= p[1];
    buf[15]= p[0];
    p = (BYTE *)&dst;
    buf[16]= p[3];
    buf[17]= p[2];
    buf[18]= p[1];
    buf[19]= p[0];

    // checksum
    chk=0;
    for(b=0;b<20;b+=2) {
        chk +=  (DWORD)buf[b+1];
        chk += ((DWORD)buf[b]) << 8;
        if(chk >= 65536L)
            chk -= 65535L; // one's complementing
//        printf("%ld\n", chk);
    }
    chk ^= 0xFFFF;
    
    buf[10] = (BYTE)(chk >> 8);
    buf[11] = (BYTE)chk;
}

void udp_header(BYTE *buf, WORD len, WORD src, WORD dst)
{
    len += 8;
    
    buf[0] = src >> 8;
    buf[1] = (BYTE)src;
    buf[2] = dst >> 8; 
    buf[3] = (BYTE)dst;
    buf[4] = len >> 8;  // length high
    buf[5] = (BYTE)len; // length low
    buf[6] = 0;
    buf[7] = 0;
}


BOOL ethernet_test(void)
{
    int t, len;
    static BYTE *tx;
    
    if(!cs8900a_detect()) {
        printf("No cs8900a detected.\n");
        return FALSE;
    }
    
    cs8900a_init();
    
    // prepare to send broadcast packet
    eth_header(&tx_buffer[0], 0x0800);
    ip_header (&tx_buffer[14], 270, 17, 0L, 0xFFFFFFFF);
    udp_header(&tx_buffer[34], 262, 68, 67);
    
    // payload
    tx = &tx_buffer[42];

    // generate DHCP Discovery
    *(tx++) = 0x01; // Boot request
    *(tx++) = 0x01; // Htype = Ethernet
    *(tx++) = 0x06; // Address length = 6
    *(tx++) = 0x00; // Hops = 0
    
    *(tx++) = 0x39; // ID
    *(tx++) = 0x03;
    *(tx++) = 0xF3;
    *(tx++) = 0x26;

    for(t=0;t<20;t++) {
        *(tx++) = 0;
    }

    for(t=0;t<6;t++) {
        *(tx++) = mac[t];
    }

    for(t=6;t<208;t++) {
        *(tx++) = 0;
    }

    *(tx++) = 0x63; // Magic cookie
    *(tx++) = 0x82; // 
    *(tx++) = 0x53; // 
    *(tx++) = 0x63; // 
    
    *(tx++) = 0x35; // DHCP Discover option
    *(tx++) = 0x01; // Length = 1
    *(tx++) = 0x01; // 

    *(tx++) = 0x3D; // DHCP Client ID option
    *(tx++) = 0x07; // Length = 7
    *(tx++) = 0x01; // 
    for(t=0;t<6;t++) {
        *(tx++) = mac[t];
    }

    *(tx++) = 0x37; // DHCP Request list
    *(tx++) = 0x06; // Length = 1

    *(tx++) = 0x01; // Subnet Mask
    *(tx++) = 0x0F; // Domain Name
    *(tx++) = 0x03; // Router
    *(tx++) = 0x06; // DNS
    *(tx++) = 0x1F; // Router discover
    *(tx++) = 0x21; // Static Route

    *(tx++) = 0xFF; // DHCP End
    *(tx++) = 0x00; // Length = 0
    *(tx++) = 0x00; // Length = 0

    // send packet
    cs8900a_tx_frame(tx_buffer, 304);
    
    // we should receive a response!
    for(t=0;t<5000;t++) {
        len = cs8900a_rx_frame(rx_buffer);
        if(len > 10) {
            printf("Frame received: Len = %d.\n", len);
            dump_hex(rx_buffer, 128);
            return TRUE;
        }
        TIMER = 250;
        while(TIMER)
            ;
    }
    printf("No frame received.\n");

    // send packet
    cs8900a_tx_frame(tx_buffer, 304);
    
    // we should receive a response!
    for(t=0;t<5000;t++) {
        len = cs8900a_rx_frame(rx_buffer);
        if(len > 10) {
            printf("Frame received: Len = %d.\n", len);
            dump_hex(rx_buffer, 128);
            return TRUE;
        }
        TIMER = 250;
        while(TIMER)
            ;
    }
    printf("No frame received.\n");


    return FALSE;
}
