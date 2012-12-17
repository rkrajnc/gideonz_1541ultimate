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
 *	 Application Main - Startup
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spi.h"
#include "sd.h"
#include "ff.h"
#include "mapper.h"
#include "gcr.h"
#include "gpio.h"
#include "keyb.h"
#include "uart.h"
#include "mem_tools.h"
#include "config.h"
#include "onewire.h"
#include "cartridge.h"
#include "version.h"
#include "last.h"
#include "d64.h"

extern FIL     	iec_file; // Using the iec spare file pointer to reduce mem consumption
extern FRESULT 	iec_fres; // Using the iec spare result to reduce mem consumption

extern BYTE    	current_drv;
extern BOOL		enable_drv;

// globally shared stuff;
FIL 	mountfile; 		// shared filepointer iec/menu both need this one.
BOOL  	disk_change = 0;// indicates that SD card was changed outside of the menu iec/menu both need this one.

BYTE    stand_alone;
DIR	    mydir;

void main_loop(void); // TODO move here?
void stand_alone_loop(void); 
void main_check_sd_change(void);

void init_sd_change(void)
{
    if(!(GPIO_IN2 & SD_CARDDET)) {
        card_det = 1;
        card_change = 1;
    }
    else {
        card_det = 0;
        card_change = 0;
    }
    printf("Startup with SD-card %spresent.\n", card_det?"":"not ");        
}

int main(void)
{
	WORD    bytes_read;
    BYTE    reu_size;
    
    BYTE    rom;
    BYTE    cart_base;
    char   *name;
/*
	BYTE    b;
    BYTE    p, *src, *dst;
    WORD    w;
*/

	memset(&mountfile, 0 ,sizeof(mountfile));
    GPIO_SET2   = DRIVE_RESET | C64_RESET;

    version_init();

    cfg_load_defaults(hardware_mask);
    cfg_load_flash();
    if(!strlen(owner_name))
        cfg_read_owner();
	
	init_sd_change();

    if(GPIO_IN & C64_CLOCK_DET) {
        printf("C64 Detected. Running Menu Mode.\n");
        stand_alone = 0;
    } else {
        printf("C64 not detected. Running Standalone Mode.\n");
        stand_alone = 1;
    }

    rom = (BYTE)cfg_get_value(CFG_1541_ROM);
    
    if(rom == 3) {
        iec_fres = f_open(&iec_file, "1541.ROM", FA_READ);
        if(iec_fres == FR_OK) {
            MAPPER_MAP1 = ROM1541_ADDR;
            printf("Managed to open 1541.rom. Now reading.\n");
            iec_fres = f_read(&iec_file, (void *)MAP1_ADDR, 8192, &bytes_read);
            MAPPER_MAP1 = MAPPER_MAP1 + 1;
            iec_fres = f_read(&iec_file, (void *)MAP1_ADDR, 8192, &bytes_read);
            f_close(&iec_file);
        } else {
            rom = 2; // switch back to 1541-II rom
        }
    }

    if(rom != 3) { // it was not loaded
        copy_page(drv_rom_loc[rom],   ROM1541_ADDR);
        copy_page(drv_rom_loc[rom]+1, ROM1541_ADDR+1);
    }        

/*
    iec_fres = f_open(&iec_file, "KCS.ROM", FA_READ);
    if(iec_fres == FR_OK) {
        MAPPER_MAP1 = KCS_ADDR;
        printf("Managed to open kcs.rom. Now reading.\n");
        iec_fres = f_read(&iec_file, (void *)0x4000, 8192, &bytes_read);
        MAPPER_MAP1 = MAPPER_MAP1 + 1;
        iec_fres = f_read(&iec_file, (void *)0x4000, 8192, &bytes_read);
        f_close(&iec_file);
    }
*/

    ClearFloppyMem();
    //d64_load_params();

    if(stand_alone) {
        disable_cartridge();
    } else {
        init_cartridge();
    }

    reu_size = (BYTE)cfg_get_value(CFG_REU_SIZE);
    SDRAM_CTRL = 0x80 | (reu_size << 4);
    
    // swap buttons configuration
    if(cfg_get_value(CFG_SWAP_BTN))
        CONFIG_BUTTONS = 0x01;
    else
        CONFIG_BUTTONS = 0x00;

    if((hardware_mask & HW_BIT_32MB) && (cfg_get_value(CFG_1541_RAMBO))) {
        CONFIG_RAMBO_EN = 1;
    } else {
        CONFIG_RAMBO_EN = 0;
    }

    GPIO_SET2   = SOUND_ENABLE;
    GPIO_CLEAR2 = C64_RESET;

    last_boot();

    cart_base = get_cart_base();

// always load for SID player development
//    if(cart_base & 0x80) { // custom cart
        name = cfg_get_string(CFG_CUST_CART);
        printf("Custom cartridge to load: %s.\n", name);

        iec_fres = f_open(&iec_file, name, FA_READ);
        if(iec_fres == FR_OK) {
            MAPPER_MAP1 = CUST_CART_ADDR;
            printf("Managed to open %s. Now reading.\n", name);
            do {
                iec_fres = f_read(&iec_file, (void *)MAP1_ADDR, 8192, &bytes_read);
                MAPPER_MAP1 = MAPPER_MAP1 + 1;
            } while(bytes_read);
            f_close(&iec_file);
        } else {
            printf("Can't open file.\n");
        }
//    }

    //console();

    if(stand_alone)
        stand_alone_loop();
        // stand alone loop never exits
    
    main_loop();
    // main loop should never exit
    
    while(1);
}
