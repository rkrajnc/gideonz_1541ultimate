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
 *	 Main loop - Polling loop after startup of the 1541 Ultimate application.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c64_menu.h"
#include "spi.h"
#include "sd.h"
#include "ff.h"
#include "mapper.h"
#include "gcr.h"
#include "gpio.h"
#include "uart.h"
#include "iec.h"
#include "keyb.h"
#include "freezer.h"
#include "screen.h"
#include "soft_signal.h"
#include "dir.h"
#include "c64_irq.h"
#include "copper.h"
#include "trace.h" 
#include "mem_tools.h"
#include "flash.h"
#include "1541.h"
#include "d64.h"
#include "dump_hex.h"
#include "copper.h"
#include "diskio.h"
#include "iec.h"
#include "config.h"
#include "buttons.h"
#include "tap.h"

BYTE    current_drv;
BYTE	enable_drv;
extern FIL     mountfile;
extern BOOL    disk_change; // indicates that SD card was changed outside menu
extern DIRECTORY menu_dir_cache;
extern BYTE    stand_alone;

BOOL frozen = FALSE;

static BOOL reinit_iec;

void console(void);

void set_drive(BYTE addr, BYTE en)
{
	current_drv = addr;
	enable_drv = en;
    printf("Set drive called.\n");

    GPIO_CLEAR  = DRIVE_ADDR_MASK;
    GPIO_SET    = ((addr&0x3) << DRIVE_ADDR_BIT);
    GPIO_SET2   = DRIVE_RESET;
	if(en) {
		DELAY5US(2);
	    GPIO_CLEAR2 = DRIVE_RESET;
        printf("Enabled drive at %d.\n", addr);
	}
}	


BYTE do_menu(BYTE boot)
{
    static BYTE reinit;

    GPIO_IMASK = 0;
    IRQ_TMR_CTRL = TMR_DISABLE;
    init_freeze();
    reinit = menu_run(boot); // reinit iec when the menu actions affected it
    frozen = FALSE;
    GPIO_IMASK = BUTTON1; // turn on interrupt on middle button
    IRQ_TMR_CTRL = TMR_ENABLE;
    
    return reinit;
}

void irq_handler(void)
{
    if(soft_irq) {   // copper irq
        do_scroller();
        scan_keyboard();
        check_sd();
        soft_irq = 0;
    }
     /* else if(((GPIO_IN & C64_IRQ)==0)&&(GPIO_IMASK & C64_IRQ)) { // another IRQn, not coming from VIC
        GPIO_SET2 = FLOPPY_INS;
        GPIO_CLEAR2 = FLOPPY_INS;
        UART_DATA = 'C';
        handle_cia_irq(); 
    } */
      else if(((GPIO_IN & BUTTON1)==0)&&(GPIO_IMASK & BUTTON1)) {
        UART_DATA = '1';
        if(!frozen) {
            frozen = TRUE;
            set_signal(SGNL_UART_BREAK);
            GPIO_IMASK = 0; // turn off all interrupts
        } else {
            UART_DATA = '#';
        }
    } else if(((GPIO_IN & BUTTON2)==0)&&(GPIO_IMASK & BUTTON2)) {
        UART_DATA = '2';
//        dump_trace();
    } else if(IRQ_TMR_CTRL & TMR_IRQ) {
        //UART_DATA = 'T';
        check_sd();
        if(stand_alone)
            read_buttons();
        IRQ_TMR_CTRL = TMR_IRQ;
    } else {
        UART_DATA = '?';
    }
}

int fastcall write(int hnd, const void *buf, unsigned count)
{
	hnd = hnd;
    return uart_write_buffer(buf, count);
}

void writeback_poll(void)
{
    static BYTE b,c;
    static BYTE skipped=0;
        
    if((((ANY_DIRTY)||(skipped))) && (card_det)) {
        if(!mountfile.org_clust) {
            printf("Error.. mount file seems to be closed.\n");
            ANY_DIRTY = 0;  // clear flag so we won't come back here endlessly
            return;
        }

        if(!skipped) {
            ANY_DIRTY = 0;  // clear flag once we didn't skip anything anymore
        }
        skipped = 0;
        for(b=0,c=0;b<41;b++) {
            if(TRACK_DIRTY(b)) {
                if((!(DRV_STATUS & DRV_MOTOR)) || ((DRV_TRACK >> 1) != b)) {
                    TRACK_DIRTY(b) = 0;
                    printf("Writing back track %d.\n", b+1);
                    d64_write_track(&mountfile, b);
                    c++;
                } else {
                    skipped ++;
                }
            }
        }
        if(c)
            if(f_sync(&mountfile) != FR_OK)
                printf("Error syncing file.\n");
    }
}

void main_check_sd_change(BYTE stand_alone)
{
    if(card_change) {
        card_change = 0; // I saw it! clear flag
        disk_change = TRUE; // notify that we've already done a disk change
		reinit_iec = TRUE;
		
        if(card_det) {
            f_mountdrv();
/*
            if(stand_alone) {
            	printf("Loading directory...");
            	dir_change(0L, &menu_dir_cache);
                menu_init();
                printf(" done.\n");
            }
*/
        }
		else {
            printf("Card removed, clearing floppy memory.\n");
            f_close(&mountfile);
            if(cfg_get_value(CFG_CLR_ON_RMV)) {
                ClearFloppyMem();
            }
        }
		ANY_DIRTY 	 = 0;  // clear flag
    }
}

void main_loop(void)
{
    BYTE iec_addr;
    BYTE iec_enable;

	BYTE cfg_drv_addr;
	BYTE cfg_drv_enable;
	
    MAPPER_MAP3 = MAP3_IO;
    GPIO_IPOL   = BUTTONS | C64_IRQ; // buttons and stuff are active low
    GPIO_IMASK  = BUTTON1;           // turn on interrupt on middle button

    IRQ_TIMER    = 18749; // 10 Hz
    IRQ_TMR_CTRL = TMR_ENABLE; // for testing card insertion
	ANY_DIRTY 	 = 0;  // clear flag
	
/*
	main_check_sd_change();

    iec_init(iec_addr, iec_enable);
*/
	menu_init();
	main_check_sd_change(0);

    /* Enable 1541 Drive */
    cfg_drv_enable = (BYTE)cfg_get_value(CFG_1541_ENABLE);
    cfg_drv_addr = (BYTE)cfg_get_value(CFG_1541_ADDR);
	set_drive(cfg_drv_addr, cfg_drv_enable);

    if(cfg_get_value(CFG_MENU_START)) {
        do_menu(1);
    }
    
    iec_enable = (BYTE)cfg_get_value(CFG_IEC_ENABLE);
    iec_addr   = (BYTE)cfg_get_value(CFG_IEC_ADDR);


    while(1) {
        main_check_sd_change(0);

   		if(reinit_iec) {
		    iec_enable = (BYTE)cfg_get_value(CFG_IEC_ENABLE);
		    iec_addr   = (BYTE)cfg_get_value(CFG_IEC_ADDR);
   			iec_init(iec_addr, iec_enable);
   			reinit_iec = FALSE;
   		}
		if(iec_enable) {
    		iec_poll();
      	}
      	
#ifdef DEVELOPMENT
        console();
#endif
        if(frozen) {
            reinit_iec = do_menu(0);

            if(((BYTE)cfg_get_value(CFG_IEC_ENABLE) != iec_enable) ||
               ((BYTE)cfg_get_value(CFG_IEC_ADDR) != iec_addr)) {

                iec_enable = (BYTE)cfg_get_value(CFG_IEC_ENABLE);
                iec_addr   = (BYTE)cfg_get_value(CFG_IEC_ADDR);
                reinit_iec = TRUE;
            }
    	    cfg_drv_enable = (BYTE)cfg_get_value(CFG_1541_ENABLE);
    	    cfg_drv_addr = (BYTE)cfg_get_value(CFG_1541_ADDR);
    
    		if((current_drv != cfg_drv_addr) || (enable_drv != cfg_drv_enable)) {
    			set_drive(cfg_drv_addr, cfg_drv_enable);
    		}
        }
        writeback_poll();
        tap_stream();
    }
}


extern DIRECTORY iec_dir_cache;
//extern WORD  iec_dir_index;   // selected entry
extern BYTE  iec_dir_depth;
//extern DWORD subdir_trace[MAX_DEPTH];
WORD  sal_index_trace[MAX_DEPTH];
WORD  sal_dir_index;

void sal_handle_left(void)
{
    static DIRENTRY *entry;

    printf("Handle left. sal Index = %d/%d.\n", sal_dir_index, iec_dir_cache.num_entries);

    if(!iec_dir_cache.num_entries)
        return;

    if(sal_dir_index)
        sal_dir_index--;

    if((sal_dir_index == 0)&&(iec_dir_cache.cbm_medium)) {
        sal_dir_index = 1; // ?
    }

    entry = dir_getentry(sal_dir_index, &iec_dir_cache);
    printf("sal Index = %d. (%s)\n", sal_dir_index, entry->fname);
}

void sal_handle_begin(void)
{
    static DIRENTRY *entry;

    printf("Handle begin. sal Index = %d/%d.\n", sal_dir_index, iec_dir_cache.num_entries);

    if(!iec_dir_cache.num_entries)
        return;

    sal_dir_index = 0;

    if((sal_dir_index == 0)&&(iec_dir_cache.cbm_medium)) {
        sal_dir_index = 1; // ?
    }

    entry = dir_getentry(sal_dir_index, &iec_dir_cache);
    printf("sal Index = %d. (%s)\n", sal_dir_index, entry->fname);
}

void sal_handle_right(void)
{
    static DIRENTRY *entry;

    printf("Handle right. sal Index = %d/%d.\n", sal_dir_index, iec_dir_cache.num_entries);

    if(!iec_dir_cache.num_entries)
        return;

    if(sal_dir_index+1 < iec_dir_cache.num_entries)
        sal_dir_index++;
    else
        sal_dir_index = iec_dir_cache.num_entries-1;

    entry = dir_getentry(sal_dir_index, &iec_dir_cache);
    printf("sal Index = %d. (%s)\n", sal_dir_index, entry->fname);
}

void sal_handle_end(void)
{
    static DIRENTRY *entry;

    printf("Handle end. sal Index = %d/%d.\n", sal_dir_index, iec_dir_cache.num_entries);

    if(!iec_dir_cache.num_entries)
        return;

    sal_dir_index = iec_dir_cache.num_entries-1;

    entry = dir_getentry(sal_dir_index, &iec_dir_cache);
    printf("sal Index = %d. (%s)\n", sal_dir_index, entry->fname);
}

BOOL sal_handle_select(BOOL)
{
    static DIRENTRY *entry;
    BYTE wp, tr, delay;
//    BOOL success;
    
    entry = dir_getentry(sal_dir_index, &iec_dir_cache);
    printf("Trying to enter %s. Depth = %d\n", entry->fname, iec_dir_depth);

    sal_index_trace[iec_dir_depth] = sal_dir_index;

    delay = (BYTE)cfg_get_value(CFG_SWAP_DELAY);
    switch(entry->ftype) {
    case TYPE_D64:
        f_close(&mountfile);
        f_open_direct(&mountfile, entry);
        wp = (BYTE)(entry->fattrib & AM_RDO);
        tr = load_d64(&mountfile, wp, delay);
        printf("Number of tracks read: %d\n", tr);
        break;
    case TYPE_GCR:
        f_close(&mountfile);
        f_open_direct(&mountfile, entry);
        wp = (BYTE)(entry->fattrib & AM_RDO);
        load_g64(&mountfile, wp, delay);
        break;
    default:
        iec_dir_down(entry);
    }

    printf("After enter: Depth = %d. Sal index = %d.\n", iec_dir_depth, sal_dir_index);

    return TRUE;
}

void sal_handle_up(void)
{
    static DIRENTRY *entry;

    printf("Handle directory up..\n");
    iec_dir_up();
    
    sal_dir_index = sal_index_trace[iec_dir_depth];

    entry = dir_getentry(sal_dir_index, &iec_dir_cache);
    printf("After up: Depth = %d. Sal index = %d. (%s)\n", iec_dir_depth, sal_dir_index, entry->fname);

/*
    if(iec_dir_depth == 0)
        return;
        
    iec_dir_depth--;
    if(iec_dir_depth == 0)
        dir_change(0L, &iec_dir_cache);
    else
        dir_change(subdir_trace[iec_dir_depth-1], &iec_dir_cache);
    
    sal_dir_index = sal_index_trace[iec_dir_depth];
    
    entry = dir_getentry(sal_dir_index, &iec_dir_cache);
    printf("Gone one level up to %s.\n", entry->fname);    
*/
}

static
void check_buttons(void)
{
    if(button_event & BUTTON0) {
        // handle left
        sal_handle_left();
        clear_button_event(BUTTON0, 0);
    } else if(button_event & BUTTON2) {
        // handle right
        sal_handle_right();
        clear_button_event(BUTTON2, 0);
    } else if(button_event & BUTTON1) {
        // handle middle
        sal_handle_select(FALSE);
        clear_button_event(BUTTON1, 0);
    } else if(button_hold & BUTTON1) {
        sal_handle_up();
        clear_button_event(0, BUTTON1);
    } else if(button_hold & BUTTON0) {
        sal_handle_begin();
        clear_button_event(0, BUTTON0);
    } else if(button_hold & BUTTON2) {
        sal_handle_end();
        clear_button_event(0, BUTTON2);
    }
/*
    if(button_hold | button_event) {
        printf("Hold = %d Event = %d.\n", button_hold, button_event);
        button_hold = 0;
        button_event = 0;
    }
*/
}

void stand_alone_loop(void)
{
    BYTE iec_addr;
    BYTE iec_enable;
	BYTE cfg_drv_addr;
	BYTE cfg_drv_enable;
    
    GPIO_SET    = MASK_BUTTONS;
    GPIO_IMASK  = 0;   // no external interrupts

    IRQ_TIMER    = 9765; // 20 Hz
    IRQ_TMR_CTRL = TMR_ENABLE; // for testing card insertion and 
	ANY_DIRTY 	 = 0;  // clear flag
	
    /* Enable 1541 Drive */
    cfg_drv_enable = (BYTE)cfg_get_value(CFG_1541_ENABLE);
    cfg_drv_addr = (BYTE)cfg_get_value(CFG_1541_ADDR);
	set_drive(cfg_drv_addr, cfg_drv_enable);
    
    /* Enable IEC drive */
    iec_enable = 1;
    iec_addr   = (BYTE)cfg_get_value(CFG_IEC_ADDR);
    if(iec_addr == current_drv)
        iec_addr++;

    reinit_iec = TRUE;
    printf("IEC address: %d\n", iec_addr);

    while(1) {
        main_check_sd_change(1);
		
   		if(reinit_iec) {
   			iec_init(iec_addr, iec_enable);
   			reinit_iec = FALSE;
   		}
		if(iec_enable) {
    		iec_poll();
        }

        check_buttons();
#ifdef DEVELOPMENT
        console();
#endif
        writeback_poll();
    }
}
