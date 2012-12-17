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
#include "personalize.h"
#include "screen.h"
#include "freezer.h"
#include "dump_hex.h"
#include "ethernet.h"

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
    UART_DATA = 'i';
    IRQ_TMR_CTRL = TMR_IRQ; // clear irq flag
    scan_keyboard();
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

BOOL string_input(char *scr, char *buffer, char maxlen)
{
    char cur = 0;
    char b;
    char len = 0;
    BYTE key;
        
/// Default to old string
    cur = strlen(buffer); // assume it is prefilled, set cursor at the end.
    if(cur > maxlen) {
        buffer[cur]=0;
        cur = maxlen;
    }
    len = cur;
/// Default to old string
    
    // place cursor
    scr[cur] |= 0x80;
    
    // loop with auto insert
    while(1) {
        key = kb_buf_getch();
        if(!key)
            continue;

        switch(key) {
        case 0x0D: // CR
            scr[cur] &= 0x7F;
            for(b=0;b<len;b++)
                buffer[b] = scr[b];
            buffer[b] = 0;
            return b!=0;
        case 0x9D: // left
            if (cur > 0) {
                scr[cur] &= 0x7F;
                cur--;
                scr[cur] |= 0x80;
            }                
            break;
        case 0x1D: // right
            if (cur < len) {
                scr[cur] &= 0x7F;
                cur++;
                scr[cur] |= 0x80;
            }
            break;
        case 0x14: // backspace
            if (cur > 0) {
                scr[cur] &= 0x7F;
                cur--;
                len--;
                for(b=cur;b<len;b++) {
                    scr[b] = scr[b+1];
                } scr[b] = 0x20;
                scr[cur] |= 0x80;
            }
            break;
        case 0x93: // clear
            for(b=0;b<maxlen;b++)
                scr[b] = 0x20;
            len = 0;
            cur = 0;
            scr[cur] |= 0x80;
            break;
        case 0x94: // del
            if(cur < len) {
                len--;
                for(b=cur;b<len;b++) {
                    scr[b] = scr[b+1];
                } scr[b] = 0x20;
                scr[cur] |= 0x80;
            }
            break;
        case 0x13: // home
            scr[cur] &= 0x7F;
            cur = 0;
            scr[cur] |= 0x80;
            break;        
        case 0x11: // down = end
            scr[cur] &= 0x7F;
            cur = len;
            scr[cur] |= 0x80;
            break;
        case 0x03: // break
            return FALSE;
        default:
            if ((key < 32)||(key > 127)) {
//                printf("unknown char: %02x.\n", key);
                continue;
            }
            if (len < maxlen) {
                scr[cur] &= 0x7F;
                for(b=len-1; b>=cur; b--) { // insert if necessary
                    scr[b+1] = scr[b];
                }
                scr[cur] = key;
                cur++;
                scr[cur] |= 0x80;
                len++;
            }
            break;
        }
//        printf("Len = %d. Cur = %d.\n", len, cur);
    }        
}

int fastcall read(int hnd, void *buf, unsigned count)
{
    static WORD map;
    int ret;
        
	hnd = hnd;
    if(!screen_initialized)
        return uart_read_buffer(buf, count);

    map = MAPPER_MAP1;
    MAPPER_MAP1 = MAP_RAMDMA;
    IRQ_TMR_CTRL = TMR_ENABLE;
    
    if(string_input((char *)cursor_pos, buf, count))
        ret = strlen(buf);
    else
        ret = 0;
        
    IRQ_TMR_CTRL = TMR_DISABLE;
    MAPPER_MAP1 = map;
    return ret;
}


BOOL test_sdram(void)
{
    const WORD pages[]   = { 0, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 70, 349, 680, 1541, 1855, 2444, 3072, 3456, 3999, 4095 };
    const WORD lengths[] = { 5, 2,  2,  2,  2,   2,   2,   2,    2,    2,  5,   7,   3,   10,   11,    3,    2,    1,    3,    1 };
    WORD i,j,e;
    
    printf("Filling (and testing)   \n");
    for(i=0;i<NR_OF_EL(pages);i++) {
        for(j=0;j<lengths[i];j++) {
            e = TestPage(1, 4096 + pages[i] + j);
            if(e) {
                printf("Errors in page %4x (%d).\n", 4096 + pages[i] + j, e);
                return FALSE;
            }
            printf(".");
        } printf("-");
    }
    printf("\nVerifying...\n");
    for(i=0;i<NR_OF_EL(pages);i++) {
        for(j=0;j<lengths[i];j++) {
            e = TestPage(0, 4096 + pages[i] + j);
            if(e) {
                printf("Errors in page %4x (%d).\n", 4096 + pages[i] + j, e);
                return FALSE;
            }
            printf(".");
        } printf("-");
    }
    printf("\n");
    return TRUE;
}

#define LOAD_DIR "toflash/"

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
                    if(!(flags & 0x04)) {
                        flash_erase_block(b);
                    }
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
    
    pages = numpages; 
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
    BYTE flags = 2;

    if(!flash_module(0x08EB,21, LOAD_DIR"bootfpga.bin", "the bootloader", 3)) { 
        printf("Verifying failed; I will now program it...\n");
	    if(!flash_module(0x08EB,21, LOAD_DIR"bootfpga.bin", "the bootloader", 3)) {
    	    if(!flash_module(0x08EB,21, LOAD_DIR"bootfpga.bin", "the bootloader", 3)) {
                bootloader_error();
    	        return FALSE;
    	    }
    	}
    }
	
    if(!flash_module(0x0808, 4, LOAD_DIR"ar5pal.bin", "Action Replay 5.0 PAL", 2)) {
        flags = 7; // virgin flash. Retry ar5
        if(!flash_module(0x0808, 4, LOAD_DIR"ar5pal.bin", "Action Replay 5.0 PAL", flags))
            return FALSE;
    }

    if(!flash_module(0x080C, 4, LOAD_DIR"ar6pal.bin", "Action Replay 6.0 PAL", flags))
        return FALSE;

    if(!flash_module(0x0810, 8, LOAD_DIR"final3.bin", "Final Cartridge III", flags))
        return FALSE;

    if(!flash_module(0x0818, 4, LOAD_DIR"sounds.bin", "Drive Sounds", 3))
        return FALSE;
    
    if(!flash_module(0x081C, 2, LOAD_DIR"sidcrt.bin", "SID Player", 3))
        return FALSE;

    if(!flash_module(0x081E, 2, LOAD_DIR"1541.bin", "CBM 1541 ROM", 3))
        return FALSE;

    if(!flash_module(0x0820, 8, LOAD_DIR"rr38pal.bin", "Retro Replay 3.8 PAL", flags))
        return FALSE;

    if(!flash_module(0x0828, 8, LOAD_DIR"ss5pal.bin", "Super Snapshot V5 PAL", flags))
        return FALSE;

    if(!flash_module(0x0830, 4, LOAD_DIR"ar5ntsc.bin", "Action Replay 5.0 NTSC", flags))
        return FALSE;

//    if(!flash_module(0x0834, 4, LOAD_DIR"ar6ntsc.bin", "Action Replay 6.0 NTSC", flags))
//        return FALSE;

    if(!flash_module(0x0834, 2, LOAD_DIR"1541C.bin", "1541C ROM", flags))
        return FALSE;

    if(!flash_module(0x0836, 2, LOAD_DIR"1541-ii.bin", "1541-II ROM", flags))
        return FALSE;

    if(!flash_module(0x0838, 8, LOAD_DIR"rr38ntsc.bin", "Retro Replay 3.8 NTSC", flags))
        return FALSE;

    if(!flash_module(0x0840, 8, LOAD_DIR"ss5ntsc.bin", "Super Snapshot V5 NTSC", flags))
        return FALSE;

//
    if(!flash_module(0x0848, 8, LOAD_DIR"tar_pal.bin", "TAsm / CodeNet PAL", flags)) {
        flags = 3;
        // retry
        if(!flash_module(0x0848, 8, LOAD_DIR"tar_pal.bin", "TAsm / CodeNet PAL", flags)) {
            return FALSE;
        }
    }

    if(!flash_module(0x0850, 8, LOAD_DIR"tar_ntsc.bin", "TAsm / CodeNet NTSC", flags))
        return FALSE;

    if(!flash_module(0x0858, 2, LOAD_DIR"epyx.bin", "Epyx Fastloader", 3))
        return FALSE;

    if(!flash_module(0x08A0,40, LOAD_DIR"appl.bin", "the application", 3))
        return FALSE;

    return TRUE;
}

int main(void)
{
    WORD idx;
    struct user_info *user;
    char line[32];

    UART_DATA = 0x21;
    
    GPIO_SET2   = DRIVE_RESET | C64_RESET;

	memset(&mountfile, 0 ,sizeof(mountfile));

    // no cart
    CART_MODE = 0x00;

    GPIO_CLEAR2 = C64_RESET;
    
    wait2sec();

    // read index
    init_freeze();
    screen_initialized = 1;
    // now we can use the console :)

    // initialize timer IRQ for keyboard scan
    IRQ_TIMER    = 3906; // 50 Hz
    
    if(!iec_test()) {
        printf("IEC cable seems faulty.\n");
        while(1);
    }

/*
    {
        WORD w;
        BYTE b;
        BYTE *mem = (BYTE *)MAP1_ADDR;
        DWORD addr = 0x1400000;
		MAPPER_MAP1 = (addr >> 13);

		w = (addr & 0x1FFF);
		for(b=0;b<32;b++) {
			if((b & 7)==0) {
				printf("\n%07lx: ", addr + (DWORD)b);
                if((w + b) >= 0x2000) {
                    MAPPER_MAP1 = MAPPER_MAP1 + 1;
                    w -= 0x2000;
                }
            }
			printf("%02x ", mem[w + b]);
		}
		printf("\n");
    }

    ethernet_test();
    while(1)
        ;
*/

    // read index
    idx = read_index();
    if(idx == 0xFFFF) {
        idx = 0;
        printf("Couldn't read index file.\n");
    }


/*
    {
        BYTE b;
        
        // dump content of first 144 bytes of one_wire rom
        onewire_reset();
        onewire_sendbyte(0xCC); // skip rom
        onewire_sendbyte(0xF0); // read memory
        onewire_sendbyte(0); // starting address
        onewire_sendbyte(0);
        
        for(b=0;b<144;b++) {
            if((b % 16)==0) {
                printf("\n0%02x: ", b);
            }
            printf("%02x ", onewire_getbyte());
        } printf("\n");
    }
    printf("!Enter your name:\n");
    read(0, line, 32);    
    printf("Your name is: %s.\n", line);
*/
    
    // get user name
    user = get_user(idx);

    printf("The unit should be a %s version.\n", user->type);
    if(!test_sdram()) {
        if(user->type[0] != 'b') {
            printf("SDRAM test failed...\n Are you sure you got a plus version?\nStop.\n");
            while(1);
        }
    } else {
        printf("SDRAM is ok!\n");
        if(user->type[0] == 'b') {
            printf(" User requested a basic version...\n Are you sure you want to continue?\n");
//            scanf("%s", line);
            read(0, line, 32);
//    		uart_read_buffer(line, 32);
    		if(line[0] != 'y') {
    		    printf("Stop.\n");
    		    while(1);
    		}
        }
    }
    
    if(user->type[0] == 'e') { // ethernet version
        if(!ethernet_test()) {
            printf("Ethernet failed.\n");
            while(1);
        }
    }

    if(!flash_roms()) {
        printf("Flashing ROMS Failed.\n");
        while(1);
    }

    if(!personalize(user)) {
        printf("PERSONALIZING FAILED... STOPPED!!\n");
        while(1);
    }
        
    // store where we left off
    if(!write_index(idx+1)) {
        printf("WARNING!! Writing index failed.\n");
    }
    
    printf("Programming cart successful...\n");

    // Init 1541 to enable sound for testing
    copy_page(0x0836, ROM1541_ADDR);
    copy_page(0x0837, ROM1541_ADDR+1);
    GPIO_CLEAR2 = DRIVE_RESET;

    while(1);
}
