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
 *	 Fuctions to communicate over the one-wire bus.
 */
#include <stdio.h>
#include "types.h"
#include "gpio.h"
#include "onewire.h"

BYTE onewire_reset(void)
{
    GPIO_SET   = ONE_WIRE;
    TIMER = 30;
    while(TIMER)
        ;
    GPIO_CLEAR = ONE_WIRE;
    TIMER = 110;
    while(TIMER)
        ;
    GPIO_SET   = ONE_WIRE;
    TIMER = 14;
    while(TIMER)
        ;
    return !(GPIO_IN & ONE_WIRE);
}

void onewire_sendbit(BYTE bit)
{
    while(TIMER)
        ;
    GPIO_CLEAR = ONE_WIRE;
    TIMER = (bit)?4:15;
    while(TIMER)
        ;
    GPIO_SET   = ONE_WIRE;
    TIMER = (bit)?8:2;
}

void onewire_sendbyte(BYTE byte)
{
    BYTE b = 200;
    while(!(GPIO_IN & ONE_WIRE) && b) {
        TIMER = 1;
        while(TIMER)
            ;
        b--;
    }
    
    for(b=0;b<8;b++) {
        onewire_sendbit(byte & 0x01); // lsb first
        byte >>= 1;
    }
}

BYTE onewire_getbyte(void)
{
    BYTE b,r,out;

    while(TIMER)
        ;

    for(out=0,b=0;b<8;b++) {
        TIMER = 14;
        while(TIMER)
            ;
        GPIO_CLEAR = ONE_WIRE;
        TIMER = 1;
        while(TIMER)
            ;
        GPIO_SET   = ONE_WIRE;
        TIMER = 1;
        while(TIMER)
            ;
        r = GPIO_IN & ONE_WIRE;
        out >>= 1;
        if(r)
            out |= 0x80;
    }
    TIMER = 10;
    return out;
}

BYTE onewire_readrom(BYTE *rom)
{
    BYTE p;

    if(onewire_reset()) {
        onewire_sendbyte(0x33);
        for(p=0;p<8;p++)
            rom[p] = onewire_getbyte();
        return 1;
    }
    return 0;
}

#ifdef DEVELOPMENT
BOOL onewire_progbuf(BYTE offset, char *buffer, BYTE len)
{
    BYTE b,crc1,crc2;
    char readbuf[8];

    if(len > 8) {
        printf("Can't program more than 8 bytes.\n");
        return FALSE;
    }

    // first read 8 bytes..
    onewire_reset();
    onewire_sendbyte(0xCC); // skip rom
    onewire_sendbyte(0xF0); // read memory
    onewire_sendbyte(offset & 0xF8); // starting address, at boundary
    onewire_sendbyte(0);
    
//    printf("reading 8 bytes from %03x: ", offset & 0xF8);
    for(b=0;b<8;b++) {
        readbuf[b] = onewire_getbyte();
//        printf("%02x ", readbuf[b]);
    }
//    printf("\n");
    
    // copy new buffer into readbuf
    for(b=0;b<len;b++) {
        readbuf[(offset+b) & 0x07] = buffer[b];
    }

    onewire_reset();
    onewire_sendbyte(0xCC); // skip rom
    onewire_sendbyte(0x0F); // write scratchpad
    onewire_sendbyte(offset & 0xF8);
    onewire_sendbyte(0);
    for(b=0;b<8;b++) {
        onewire_sendbyte(readbuf[b]);
    }
//    crc1 = onewire_getbyte();
//    crc2 = onewire_getbyte();

    onewire_reset();
    onewire_sendbyte(0xCC); // skip rom
    onewire_sendbyte(0x55); // copy scratchpad
    onewire_sendbyte(offset & 0xF8);
    onewire_sendbyte(0);
    onewire_sendbyte(7); // AA=0, PF=0, E2:0=111
    onewire_waitprog();
    crc1 = onewire_getbyte();
    crc2 = onewire_getbyte();
    if((crc2 != 0xAA)&&(crc2 != 0x55)) { // should toggle to indicate correct programming
        return FALSE;
    }
    return TRUE;
}
#endif

BYTE onewire_readmem(BYTE offset)
{
    BYTE p;

    if(onewire_reset()) {
        onewire_sendbyte(0xCC); // skip rom
        onewire_sendbyte(0xF0); // read memory
        onewire_sendbyte(offset); // starting address
        onewire_sendbyte(0); // high byte
        p = onewire_getbyte();
        return p;
    }
    return 0;
}

#ifndef BOOT
void onewire_waitprog(void)
{
    BYTE p;
    for(p=0;p<12;p++) {
        TIMER = 250;
        while(TIMER)
            ;
    }
}
#endif
