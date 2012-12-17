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
 *   Functions that initiate the freezing and can exit the freezer gracefully
 */
	
#include "manifest.h"
#include <stdio.h>
#include <stdlib.h>
#include "freezer.h"
#include "ff.h"
#include "mapper.h"
#include "screen.h"
#include "gpio.h"
#include "c64_irq.h"
#include "c64_addr.h"
#include "keyb.h"
#include "cartridge.h"

#pragma codeseg ("DMACODE")

/*
 * Memory map of backup area (8K):
 * 0000 - 0FFF  Character 'ROM'  (read from SD card)
 * 1000 - 1BFF  Backup of C64 memory (3KB)
 * 1C00 - 1FE7  Backup of C64 color  (1000B)
 */

#define BACKUP_SIZE 3072

#define MEM_LOC(x)   *((unsigned char *)(SCREEN1_ADDR + x))        // (MAP 400 << 13) + 0400 = 0800400
#define CHAR_DEST(x) *((unsigned char *)(MAP1_ADDR + 0x0800 + x))  // (MAP 400 << 13) + 0800 = 0800800

// MAP2 locations (pointing to SRAM)
#define CHAR_SRC(x)  *((unsigned char *)(MAP2_ADDR + x))           // (MAP 038 << 13) + 0000 = 0070000
#define BCK_LOC(x)   *((unsigned char *)(MAP2_ADDR + 0x1000 + x))  // (MAP 038 << 13) + 1000 = 0071000
#define BCK_COLOR(x) *((unsigned char *)(MAP2_ADDR + 0x1C00 + x))  // (MAP 038 << 13) + 1C00 = 0071C00

#define CIA1_REG(x)  *((unsigned char *)(MAP3_ADDR + 0x0C00 + x))
#define CIA2_REG(x)  *((unsigned char *)(MAP3_ADDR + 0x0D00 + x))


#define NUM_VICREGS 48

BYTE BCK_VIC[NUM_VICREGS];
BYTE BCK_CIA[5];

BYTE vic_irq_en, vic_irq, raster, raster_hi;
BYTE stop_mode;
BYTE vic_d012;
BYTE vic_d011;

void determine_d012(BYTE *b12, BYTE *b11)
{
    BYTE b;
    // pre-condition: C64 is already in the stopped state, and IRQ registers have already been saved
    //                C64 IRQn to SD-IRQ is not enabled.
    
    VIC_REG(26) = 0x01; // interrupt enable = raster only
    VIC_REG(25) = 0x81; // clear raster interrupts
    
    // poll until VIC interrupt occurs

    *b12 = 0;
    *b11 = 0;

    b = 0;
    while(!(VIC_REG(25) & 0x81)) {
        if(!TIMER) {
            TIMER = 200;
            b++;
            if(b == 40)
                break;
        }
    }

    *b12 = VIC_REG(18); // d012
    *b11 = VIC_REG(17); // d011
    
    VIC_REG(26) = 0x00; // disable interrutps
    VIC_REG(25) = 0x81; // clear raster interrupt
}

void stop_c64(BOOL do_raster)
{
    BYTE b;
    WORD w;
    
	// stop the c-64
    stop_mode = 0;
    raster = 0;
    raster_hi = 0;
    vic_irq_en = 0;
    vic_irq = 0;

    // First we try to break on the occurence of a bad line.
    GPIO_CLEAR  = STOP_COND; // cond = 0

    if(do_raster) {
        // wait maximum for 25 ms (that is 1/40 of a second; there should be a bad line in this time frame
        TIMER = 200; // 1 ms
    
    	// request to stop the c-64
    	GPIO_SET2 = C64_STOP;
        
        for(b=0;b<25;) {
            if(GPIO_IN2 & C64_STOP) {  // has the C64 stopped already?
    //            VIC_REG(48) = 0;  // switch to slow mode (C128)
            	// enter ultimax mode, so we can be sure that we can access IO space!
            	GPIO_SET2	= C64_GAME;
            	GPIO_CLEAR2 = C64_EXROM;

                raster     = VIC_REG(18);
                raster_hi  = VIC_REG(17);
                vic_irq_en = VIC_REG(26);
                vic_irq    = VIC_REG(25);
                stop_mode = 1;
                break;
            }
            if(!TIMER) {
                TIMER = 200; // 1 ms
                b++;
            }
        }
    }
    
    // If that fails, the screen must be blanked or so, so we try to break upon a safe R/Wn sequence
    if(!stop_mode) {
        GPIO_CLEAR = STOP_COND;
        GPIO_SET   = (1 << STOP_COND_BIT); // set to mode 1

    	// request to stop the c-64
    	GPIO_SET2 = C64_STOP;  // we do this again, in case we need to stop without raster

        TIMER = 200; // 1 ms
    
        for(w=0;w<100;) { // was 1500
            if(GPIO_IN2 & C64_STOP) {
                stop_mode = 2;
                break;
            }
            if(!TIMER) {
                TIMER = 200; // 1 ms
                w++;
            }
        }
    }

    // If that fails, the CPU must have disabled interrupts, and be in a polling read loop
    // since no write occurs!
    if(!stop_mode) {
        GPIO_SET = (3 << STOP_COND_BIT); // set to mode 3

        // wait until it is stopped (it always will!)
        while(!(GPIO_IN2 & C64_STOP))
            ;

        stop_mode = 3;
    }

    if(do_raster) {  // WRONG parameter actually, but this function is only called in two places:
                     // entering the menu, and dma load. menu calls it with true, dma load with false.... :-S
                     // TODO: Clean up
        switch(stop_mode) {
            case 1:
                printf("Frozen on Bad line. Raster = %02x. VIC Irq Enable: %02x. Vic IRQ: %02x\n", raster, vic_irq_en, vic_irq);
                if(vic_irq_en & 0x01) { // Raster interrupt enabled
                    determine_d012(&vic_d012, &vic_d011);
                    printf("Original d012/11 content = %02x %02X.\n", vic_d012, vic_d011);
                } else {
                    vic_d011 = VIC_REG(17); // for all other bits
                }
                break;
            case 2:
                printf("Frozen on R/Wn sequence.\n");
                break;
            case 3:
                printf("Hard stop!!!\n");
                break;
            default:
                printf("Internal error. Should be one of the cases.\n");
        }
    }
}

void run_c64(void)
{
    static BYTE dummy;
    static BYTE rast_lo;
    static BYTE rast_hi;
    
    rast_lo = raster - 1;
    rast_hi = raster_hi & 0x80;
    if(!raster)
        rast_hi ^= 0x80;

    // either resume in mode 0 or in mode 3  (never on R/Wn sequence of course!)
    if (stop_mode == 1) {
        // this can only occur when we exit from the menu, so we are still
        // in ultimax mode, so we can see the VIC here.
        if(vic_irq & 0x01) { // was raster flag set when we stopped? then let's set it again
            VIC_REG(26) = 0x81;
            VIC_REG(18) = rast_lo;
            VIC_REG(17) = rast_hi;
            while((VIC_REG(18) != rast_lo)&&((VIC_REG(17)&0x80) != rast_hi)) {
                if(!TIMER) {
                    TIMER = 200;
                    dummy++;
                    if(dummy == 40)
                        break;
                }
            }
            VIC_REG(18) = vic_d012;
            VIC_REG(17) = vic_d011;
        } else {
            while((VIC_REG(18) != rast_lo)&&((VIC_REG(17)&0x80) != rast_hi)) {
                if(!TIMER) {
                    TIMER = 200;
                    dummy++;
                    if(dummy == 40)
                        break;
                }
            }
            VIC_REG(18) = vic_d012;
            VIC_REG(17) = vic_d011;
            VIC_REG(25) = 0x81; // clear raster irq
        }
        VIC_REG(26) = vic_irq_en;

        raster     = VIC_REG(18);
        vic_irq_en = VIC_REG(26);
        vic_irq    = VIC_REG(25);

        GPIO_CLEAR = STOP_COND;

        // return to normal mode
        GPIO_CLEAR2 = C64_EXROM | C64_GAME;

        // dummy cycle
        dummy = VIC_REG(0);
        
    	// un-stop the c-64
    	GPIO_CLEAR2	= C64_STOP;

        printf("Resumed on Bad line. Raster = %02x. VIC Irq Enable: %02x. Vic IRQ: %02x\n", raster, vic_irq_en, vic_irq);
    } else {
        GPIO_SET = STOP_COND;
    	// un-stop the c-64
        // return to normal mode
        GPIO_CLEAR2 = C64_EXROM | C64_GAME;

    	GPIO_CLEAR2	= C64_STOP;
    }
}

/*
-------------------------------------------------------------------------------
							init_freeze (split in subfunctions)
							===========
  Abstract:

	Stops the C64. Backups the resources the cardridge is allowed to use.
	Clears the screen and sets the default colours.
	
-------------------------------------------------------------------------------
*/
void backup_c64_io(void)
{
	WORD w;
    BYTE b;
	char *scr = (char *)SCREEN1_ADDR;


	// enter ultimax mode, as this might not have taken place already!
	GPIO_SET2	= C64_GAME;
	GPIO_CLEAR2 = C64_EXROM;

	// back up VIC registers
	for(b=0;b<NUM_VICREGS;b++)
		BCK_VIC[b] = VIC_REG(b); 
	
	// now we can turn off the screen to avoid flicker
	VIC_CTRL	= 0;
	BORDER		= 0; // black
	BACKGROUND	= 0; // black for later
    SID_VOLUME  = 0;
	
    // have a look at the timers.
    // These printfs introduce some delay.. if you remove this, some programs won't resume well. Why?!
    printf("CIA1 registers: ");
    for(b=0;b<13;b++) {
        printf("%02x ", CIA1_REG(b));
    } printf("\n");

	// back up 3 kilobytes of RAM to do our thing 
	MAPPER_MAP1 = MAP_RAMDMA;
	MAPPER_MAP2 = MAP_BACKUP;
	
	for(w=0;w<BACKUP_SIZE;w++) {
		BCK_LOC(w) = MEM_LOC(w);
	}

	// backup color ram
	MAPPER_MAP2 = MAP_BACKUP;
	for(w=0;w<1000;w++) {
		BCK_COLOR(w) = COLOR_MAP3(w);
	}

	// now copy our own character map into that piece of ram at 0800
	MAPPER_MAP1 = MAP_RAMDMA;
	for(w=0;w<2048;w++) {
		CHAR_DEST(w) = CHAR_SRC(w);
	}
	
	// backup CIA registers
	BCK_CIA[0] = CIA2_DDRA;
	BCK_CIA[1] = CIA2_DPA;
	BCK_CIA[2] = CIA1_DDRA;
	BCK_CIA[3] = CIA1_DDRB;
	BCK_CIA[4] = CIA1_DPA;
}

void init_c64_io(void)
{
//    BYTE b;
    
    // set VIC bank to 0
	CIA2_DDRA |= 0x03;
	CIA2_DPA  |= 0x03;

    // enable keyboard
	CIA1_DDRA  = 0xFF; // all out
	CIA1_DDRB  = 0x00; // all in
	
	// Set VIC to use charset at 0800
	// Set VIC to use screen at 0400
	VIC_MMAP = 0x12; // screen $0400 / charset at 0800

	start_pos  = 0x0000;
	width	   = 40;
	num_lines  = 25;
	cursor_pos = MAP1_ADDR + 0x0400;
	cursor_x   = 0;
	cursor_y   = 0;
	do_color   = 0;
	clear_screen(0x0f);
	
    // init_cia_handler clears the state of the cia interrupt handlers.
    // Because we cannot read which interrupts are enabled,
    // the handler simply catches the interrupts and logs which interrupts must
    // have been enabled. These are disabled consequently, and enabled again
    // upon leaving the freezer.
	init_cia_handler();
//    printf("cia irq mask = %02x. Test = %02X.\n", cia1_imsk, cia1_test);

/* debug 
    printf("CIA1 registers: (after enabling keyboard)");
    for(b=0;b<13;b++) {
        printf("%02x ", CIA1_REG(b));
    } printf("\n");
    for(b=0;b<13;b++) {
        printf("%02x ", CIA2_REG(b));
    } printf("\n");

    // init according to C64 rom
    b = 0;
    CIA1_REG(13) = 0x7F; // IRQ control reg
    b += VIC_REG(0);
    CIA2_REG(13) = 0x7F; // NMI control reg
    b += VIC_REG(0);
    CIA1_REG(0)  = 0x7F; // Data port A
    b += VIC_REG(0);
    CIA1_REG(14) = 0x08; // Ctrl Reg A
    b += VIC_REG(0);
    CIA2_REG(14) = 0x08; // Ctrl Reg A
    b += VIC_REG(0);
    CIA1_REG(15) = 0x08; // Ctrl Reg B
    b += VIC_REG(0);
    CIA2_REG(14) = 0x08; // Ctrl Reg A
    b += VIC_REG(0);
    
	CIA1_DDRB  = 0xAA;
    b += VIC_REG(0);
	CIA1_DDRA  = 0xFF; // all out
    b += VIC_REG(0);

    printf("CIA1 registers: (after init)");
    for(b=0;b<13;b++) {
        printf("%02x ", CIA1_REG(b));
    } printf("\n");

	CIA1_DDRB  = 0x00; // all in
*/

    VIC_REG(17) = 0x1B; // vic_ctrl	// Enable screen in text mode, 25 rows
	VIC_REG(18) = 0xF8; // raster line to trigger on
	VIC_REG(21) = 0x00; // turn off sprites
    VIC_REG(22) = 0xC8; // Screen = 40 cols with correct scroll
	VIC_REG(26) = 0x01; // Enable Raster interrupt
}    


void init_freeze(void)
{
    stop_c64(TRUE);

    // turn off button interrupts on SD-CPU
    GPIO_IMASK  &= ~BUTTONS;

    backup_c64_io();
    init_c64_io();
}

/*
-------------------------------------------------------------------------------
							exit_freeze (split in subfunctions)
							===========
  Abstract:

	Restores the backed up resources and resumed the C64
	
-------------------------------------------------------------------------------
*/

void restore_c64_io(void)
{
    BYTE b;
    WORD w;

    // disable screen
    VIC_CTRL = 0;

    MAPPER_MAP1 = MAP_RAMDMA;
    MAPPER_MAP2 = MAP_BACKUP;

    // restore memory
    for(w=0;w<BACKUP_SIZE;w++) {
        MEM_LOC(w) = BCK_LOC(w);
    }

    // restore color ram
    for(w=0;w<1000;w++) {
        COLOR_MAP3(w) = BCK_COLOR(w);
    }

    // restore the cia registers
    CIA2_DDRA = BCK_CIA[0];
    CIA2_DPA  = BCK_CIA[1];
	CIA1_DDRA = BCK_CIA[2];
	CIA1_DDRB = BCK_CIA[3];
	CIA1_DPA  = BCK_CIA[4];

    printf("Set CIA1 %02X %02X %02X %02X %02X\n", BCK_CIA[0], BCK_CIA[1], BCK_CIA[2], BCK_CIA[3], BCK_CIA[4]);

    // restore vic registers
    for(b=0;b<NUM_VICREGS;b++) {
        VIC_REG(b) = BCK_VIC[b]; 
    }        

    restore_cia();  // Restores the interrupt generation

    SID_VOLUME = 15;  // turn on volume. Unfortunately we could not know what it was set to.
    SID_DUMMY  = 0;   // clear internal charge on databus!
}

void exit_freeze(void)
{
    wait_kb_free();
    
    // disable SD_CPU IRQ interrupt
    GPIO_IMASK &= ~C64_IRQ;
    
    restore_c64_io();

    // resume C64
    run_c64();
    
    // turn on interrupt again on middle button
    GPIO_IMASK |= BUTTON1;
}


extern BYTE *dmacode_start;
extern BYTE *dmacode_end;

void dma_load_init(void)
{
    static WORD i;
    static BYTE *s, *d;
    static WORD vec;
    static BYTE b;
    
    MAPPER_MAP1 = MAP_RAMDMA;

    GPIO_IMASK  = 0;    // turn off all interrupts
    disable_cartridge();

    // reset c64
    GPIO_SET2   = C64_RESET;
    GPIO_SET2   = C64_EXROM; // TODO: Conditional? In 128 mode??
    GPIO_CLEAR2 = C64_GAME;
    
    TIMER     = 200;
    while(TIMER)
        ;

    GPIO_CLEAR2 = C64_RESET;

    // wait until c64 is ready  (we know that when the address 0x0315 is written to another value)

    TIMER     = 255;
    while(TIMER)
        ;

    GPIO_CLEAR2 = C64_EXROM; // 128 should have tested already (TODO)

    stop_c64(FALSE);

    // init test byte
    *(BYTE *)(MAP1_ADDR + 0x0315) = 0x00;

    while((*(BYTE *)(MAP1_ADDR + 0x0315)) == 0x00) {
        run_c64();
        for(b=0;b<20;b++)  {
            TIMER = 255;
            while(TIMER)
                ;
        }
        printf("/");
        stop_c64(FALSE);
    }

    // Copy loader hook onto stack of C64.
    
    i = (WORD)&dmacode_end - (WORD)&dmacode_start;
    s = (BYTE *)&dmacode_start;
    d = (BYTE *)(MAP1_ADDR + 0x0140);
        
    while(i) {
        *(d++) = *(s++);
        i--;
    }

    run_c64();
    // wait a little bit more
    for(i=0;i<200;i++) {
        TIMER = 200;
        while(TIMER)
            ;
    }

    stop_c64(FALSE);
    
    // backup soft IRQ vector
    vec = *(WORD *)(MAP1_ADDR + 0x0314);
    *(WORD *)(MAP1_ADDR + 0x0140) = vec;
    
    printf("Vector read = %04x\n", vec);

    // set IRQ vector temporarily
    *(WORD *)(MAP1_ADDR + 0x0314) = 0x0146;

    // wait until C64 has actually switched to IRQ.
    while((*(BYTE *)(MAP1_ADDR + 0x0143)) == 0x00) {
        run_c64();
        TIMER = 255;
        while(TIMER)
            ;
        printf("_");
        stop_c64(FALSE);
    }
}

extern BYTE current_drv;


void dma_load_exit(BYTE run_code)
{
    // signal done to dma program in C64
    MAPPER_MAP1 = MAP_RAMDMA;
    *(BYTE *)(MAP1_ADDR + 0x0142) = run_code;

    // fix drive number
    *(BYTE *)(MAP1_ADDR + 0x00BA) = current_drv;
    // fix load status
    *(BYTE *)(MAP1_ADDR + 0x0090) = 0x40;
    // fix FRESPC pointer
    *(WORD *)(MAP1_ADDR + 0x0035) = 0xA000;
    // fix EAL (End of load)
    *(WORD *)(MAP1_ADDR + 0x00AE) = *(WORD *)(MAP1_ADDR + 0x002D);
    
    enable_cartridge();
    TIMER     = 255;
    while(TIMER)
        ;
    CART_KILL = 0xFF; // kill it immediately, so it would only work after reset
    TIMER     = 255;
    while(TIMER)
        ;

    run_c64();
}
