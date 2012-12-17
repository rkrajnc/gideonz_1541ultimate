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
 *	 Utility to decode IEC trace data into readable bytes, for debug
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../../data/inc/types.h"

struct data_entry {
    WORD stamp;
    WORD aux;
    WORD data;
};

#define DEBUG     0x000F
#define CART_SRQ  0x0010
#define DRV_ATN   0x0020
#define DRV_DATA  0x0040
#define DRV_CLK   0x0080
#define CART_AUTO 0x0100
#define CART_ATN  0x0200
#define CART_DATA 0x0400
#define CART_CLK  0x0800
#define BUS_SRQ   0x1000
#define BUS_ATN   0x2000
#define BUS_DATA  0x4000
#define BUS_CLK   0x8000

enum t_state { idle, host_send, hs_wait_drive_ready, transfer, transfer_done, warp };

enum t_state state = idle;

const BYTE match_pattern[] = "M-E";
const int  match_length    = 3;

void handle_event(WORD data, WORD event, DWORD time, WORD delta, FILE *fo)
{
    static int count;
    static int databyte;
    static int atn;
    static int match_cnt;
    static BOOL match_found = FALSE;
    static BOOL warping = FALSE;
    static int bit_cnt;
    static int warp_addr;
    static WORD  last_data;
    static DWORD clkedgeat = 0;
    static BOOL  clk_proc = FALSE;
        
    if ((event & DRV_CLK) && (data & DRV_CLK) && match_found && !warping) {
        printf("Time = %d. Now going in warp transfer mode.\n", time);
        state = warp;
        warping = TRUE;
        bit_cnt = 8;
        warp_addr = 0x56;
        return;
    }

    switch(state) {
    case idle:
        if (!(data & BUS_CLK)) {
            state = host_send;
        }
        break;
    case host_send:
        if ((data & BUS_CLK) && !(data & BUS_DATA)) {
            state = hs_wait_drive_ready;
        }
        break;
    case hs_wait_drive_ready:
        if (data & BUS_DATA) {
//            printf("Goto transfer @ %6ld.", time);
            atn = !(data & BUS_ATN);
            state = transfer;
            count = 8;
            databyte = 0;
        }            
        break;
    case transfer:
        if((event & BUS_CLK)&&(data & BUS_CLK)) {  // rising edge of clock
            databyte >>= 1;
            if(data & BUS_DATA) {
                databyte |= 0x80;
            }
            count --;
            if(!count) {
                state = transfer_done;
            }
        }
        break;
    case transfer_done:
        if((event & DRV_DATA) && !(data & DRV_DATA)) {
            printf("%c%02x %c\n", (atn)?'!':' ', databyte, (databyte >= 32)?databyte:'?', time);
//            printf("dr ack. Data = %c%02x %c (time = %6ld)\n", (atn)?'!':' ', databyte, (databyte >= 32)?databyte:'?', time);
//            printf("dr ack. Data = %c%02x %c \n", (atn)?'!':' ', databyte, (databyte >= 32)?databyte:'?');
            state = host_send;

            if(databyte == match_pattern[match_cnt]) {
                match_cnt++;
                if(match_cnt == match_length) {
                    match_found = TRUE;
                    printf("PATTERN FOUND!\n");
                }
            } else {
                match_cnt = 0;
            }
        }
        break;
        
    case warp:
        if((!clk_proc)&&(clkedgeat)) {
            if(time > clkedgeat + 5) { // sample data after 5 us after last clock edge
                if (bit_cnt == 8) {
                    printf("Byte start at %7ld (delta=%3d): ", time, delta);
                }

                // now shift data with last data bit seen
                databyte >>= 1;
                if(!last_data) {
                    databyte |= 0x80;
                }            
                bit_cnt--;
                if(!bit_cnt) {
                    printf("%04x %02x\n", warp_addr, databyte);
                    fputc(databyte, fo);
                    warp_addr ++;
                    bit_cnt = 8;
                }
                clk_proc = TRUE;
            }
        }
        if (event & BUS_CLK) {
            clkedgeat = time;
            clk_proc  = FALSE;
        }
        last_data = (data & BUS_DATA);
        break;
        
    default:
        printf("unknown state!\n");
    }
}

void trace_decode(FILE *fi, FILE *fo)
{
    DWORD time = 0;
    WORD  prev, current, change;
    int   r,i;
    struct data_entry d;
    BYTE   b;
        
    for(i=0;;i++) {
        r = fread(&d, 1, 6, fi);
        if(r != 6)
            break;
        if(d.aux == 0xAAAA)
            break;

        if(i==0)
            prev = d.data;
        else
            time += (d.stamp & 0x7FFF) + 1;

        handle_event(d.data, d.data ^ prev, time, (d.stamp & 0x7FFF) + 1, fo);
        prev = d.data;
    }
}

int main(int argc, char **argv)
{
    FILE *fi, *fo;
    
    if(argc != 2) {
        printf("Usage: decode <file>\n");
        exit(1);
    }
    fi = fopen(argv[1], "rb");
    fo = fopen("warp_out.prg", "wb");
    if(!fo) {
        fprintf(stderr, "Can't open out file.\n");
        exit(3);
    } else {
        fputc(0x56, fo);
        fputc(0, fo);
    }
    if(fi) {
        trace_decode(fi, fo); 
    } else {
        fprintf(stderr, "Can't open file.\n");
        exit(2);
    }
    fclose(fo);
    fclose(fi);
    return 0;
}
