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
#include "flash.h"
#include "freezer.h"
#include "screen.h"
#include "dump_hex.h"

FIL     prog_file; // Using the iec spare file pointer to reduce mem consumption
FRESULT prog_fres; // Using the iec spare result to reduce mem consumption

// globally shared stuff;
FIL 	mountfile; 		// shared filepointer iec/menu both need this one.
BOOL  	disk_change = 0;// indicates that SD card was changed outside of the menu iec/menu both need this one.

DIR	    mydir;

BYTE screen_initialized = 0;
void main_check_sd_change(void);

// satisfying the linker
BYTE plus_version = 0;
BYTE current_drv = 8;
void *dmacode_start;
void *dmacode_end;
// end linker satisfaction

void irq_handler(void)
{
}

void wait2sec(void)
{
    WORD g = 2000;
    while(g) {
        g--;
        TIMER = 200;
        while(TIMER)
            ;
    }
}


int fastcall write(int hnd, const void *buf, unsigned count)
{
	static BYTE *b;
	static int c;

    b = buf;
    c = (int)count;

	hnd = hnd;
    if(!screen_initialized)
        return uart_write_buffer(buf, count);
    while(c) {
        char_out(*b);
        b++;
        c--;
    }
    return count;
}

#define LOAD_DIR "update1/"

BOOL flash_module(WORD page, WORD numpages, char *fn, char *name, BYTE flags)
{
    int i;
    static WORD bytes_read;
    static WORD b, last_b = 0;
    static long addr;
    static WORD pages;
    
    pages = numpages;
    if(flags & 0x01) {
        prog_fres = f_open(&prog_file, fn, FA_READ);
        if(prog_fres == FR_OK) {
            printf("Flashing %s...", name);
            i = page;
            do {
                MAPPER_MAP1 = 0;
                FillPage(0xff, 0);
                prog_fres = f_read(&prog_file, (void *)0x4000, 8192, &bytes_read);
                addr = (((long)(i & 0x7FF)) << 13);
                b = flash_addr2block(addr);
                if(b != last_b) {
                    flash_erase_block(b);
                    last_b = b;
                }
                flash_page(0, i);
                i++;
                pages--;
            } while((bytes_read == 8192)&&(pages));
            f_close(&prog_file);
            printf(" OK!\n");
        } else {
            printf("\nCouldn't open %s:\n-> Not flashed!\n", fn);
            return FALSE;
        }
    }
    
    pages = numpages; // should fail
    if(flags & 0x02) {
        prog_fres = f_open(&prog_file, fn, FA_READ);
        if(prog_fres == FR_OK) {
            printf("Verifying %s...", name);
            i = page;
            do {
                MAPPER_MAP1 = 0;
                FillPage(0xff, 0);
                prog_fres = f_read(&prog_file, (void *)0x4000, 8192, &bytes_read);
//                if(bytes_read != 0x2000) {
//                    printf("Can't read 8192 bytes from file, but %d (error %d)\n", bytes_read, prog_fres);
//                }
                if(b=verify_page(0, i)) { // return 0 on good, address when wrong
                    printf(" ERROR!\n",b);
                    dump_hex((void*)(b&0xFFF0), 16);
                    b ^= 0x2000;
                    dump_hex((void*)(b&0xFFF0), 16);
                    return FALSE;
                }
                i++;
                pages--;
            } while((bytes_read == 8192)&&(pages));
            f_close(&prog_file);
            printf(" OK!\n");
        } else {
            printf("\nCouldn't open %s:\n-> Not verified!\n", fn);
            return FALSE;
        }
    }
    return TRUE;
}

void bootloader_error(void)
{
    printf("\nProgramming the bootloader failed.\n");
    printf("It is likely that your 1541U got brickedand needs to be sent back.\n");
    printf("Please contact info@1541ultimate.net\n");
}

BOOL flash_roms(void)
{
    char version;
#ifndef NO_BOOT
	if(onewire_readmem(0x5F) != 0x15) {
	    if(!flash_module(0x08EB,21, LOAD_DIR"bootfpga.bin", "the bootloader", 3)) {
    	    if(!flash_module(0x08EB,21, LOAD_DIR"bootfpga.bin", "the bootloader", 3)) {
                bootloader_error();
    	        return FALSE;
    	    }
        }	

	    // set new bootloader version!
	    version = 0x15; // 1.5
	    onewire_progbuf(0x5F, &version, 1);
    	if(onewire_readmem(0x5F) != 0x15) {  // just to make sure
    	    onewire_progbuf(0x5F, &version, 1);
	    }
	} else {  // version number is correct
        printf("Correct bootloader version..\n");
/*
	    if(!flash_module(0x08EB,21, LOAD_DIR"bootfpga.bin", "the bootloader", 2)) { // verify only!
            printf("Verifying failed; I will now program it...\n");
    	    if(!flash_module(0x08EB,21, LOAD_DIR"bootfpga.bin", "the bootloader", 3)) {
        	    if(!flash_module(0x08EB,21, LOAD_DIR"bootfpga.bin", "the bootloader", 3)) {
                    bootloader_error();
        	        return FALSE;
        	    }
        	}
        }
*/
    }
#endif
	
    if(!flash_module(0x0808, 4, LOAD_DIR"ar5pal.bin", "Action Replay 5.0 PAL", 3))
        return FALSE;

    if(!flash_module(0x080c, 4, LOAD_DIR"ar6pal.bin", "Action Replay 6.0 PAL", 3))
        return FALSE;

    if(!flash_module(0x0810, 8, LOAD_DIR"final3.bin", "Final Cartridge III", 3))
        return FALSE;
    
    if(!flash_module(0x0818, 6, LOAD_DIR"sounds.bin", "Drive Sounds", 3))
        return FALSE;
    
    if(!flash_module(0x081C, 6, LOAD_DIR"sidcrt.bin", "SID Player", 3))
        return FALSE;

    if(!flash_module(0x081E, 2, LOAD_DIR"1541.bin", "CBM 1541 ROM", 3))
        return FALSE;

    if(!flash_module(0x0820, 8, LOAD_DIR"rr38pal.bin", "Retro Replay 3.8 PAL", 3))
        return FALSE;

    if(!flash_module(0x0828, 8, LOAD_DIR"ss5pal.bin", "Super Snapshot V5 PAL", 3))
        return FALSE;

    if(!flash_module(0x0830, 4, LOAD_DIR"ar5ntsc.bin", "Action Replay 5.0 NTSC", 3))
        return FALSE;

//    if(!flash_module(0x0834, 4, LOAD_DIR"ar6ntsc.bin", "Action Replay 6.0 NTSC", 3))
//        return FALSE;

    if(!flash_module(0x0834, 2, LOAD_DIR"1541C.bin", "1541C ROM", 3))
        return FALSE;

    if(!flash_module(0x0836, 2, LOAD_DIR"1541-ii.bin", "1541-II ROM", 3))
        return FALSE;

    if(!flash_module(0x0838, 8, LOAD_DIR"rr38ntsc.bin", "Retro Replay 3.8 NTSC", 3))
        return FALSE;

    if(!flash_module(0x0840, 8, LOAD_DIR"ss5ntsc.bin", "Super Snapshot V5 NTSC", 3))
        return FALSE;

    if(!flash_module(0x0848, 8, LOAD_DIR"tar_pal.bin", "TAsm / CodeNet PAL", 3))
        return FALSE;

    if(!flash_module(0x0850, 8, LOAD_DIR"tar_ntsc.bin", "TAsm / CodeNet NTSC", 3))
        return FALSE;

    if(!flash_module(0x0858, 2, LOAD_DIR"epyx.bin", "Epyx Fastloader", 3))
        return FALSE;

    if(!flash_module(0x08A0,40, LOAD_DIR FLASH_APPL, "the application", 3))
        return FALSE;

    return TRUE;
}


int main(void)
{
    char *appl;

    UART_DATA = 0x21;
    
    GPIO_SET2   = DRIVE_RESET | C64_RESET;

	memset(&mountfile, 0 ,sizeof(mountfile));

    // no cart
    CART_MODE = 0x00;

    printf("Even wachten..\n");
    wait2sec();

    GPIO_CLEAR2 = C64_RESET;
    
    printf("Nog even wachten..\n");
    wait2sec();

//    console();
    // read index
    printf("Freeze!!\n");
    init_freeze();
    screen_initialized = 1;
    // now we can use the console :)

/* Reset factory default settings */    
    cfg_load_defaults(0xFF);
    cfg_load_flash();
    appl = cfg_get_string(CFG_BOOT_FILE);
    printf("The boot application is %s.\n", appl);
    
    printf("Resetting configuration data..\n");
    flash_erase_block(flash_get_cfg_sector());
    cfg_load_defaults(0xFF);
    cfg_read_owner();
    cfg_save_flash();

/* Flash new firmware */
    if(!flash_roms()) {
        printf("FLASHING FAILED... STOPPED!!\n");
        while(1);
    }
    
    printf("Programming cart successful...\n");

    if(strcmp(appl, "appl.bin")==0) { // if default name, then delete
        printf("Deleting boot application from root... ");
        prog_fres = f_unlink(appl);
        if(prog_fres == FR_OK)
            printf("\nDone!");
        else
            printf("\nFailed.\n\n"
                   "Please remove '%s' from the root\n"
                   "directory before proceeding.", appl);
    }

    printf("\n\nStopped.");
    while(1);
}
