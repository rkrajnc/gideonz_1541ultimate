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
 *	 Debug console of the 1541 Ultimate. This console presents itself
 *   on the UART. Note that the main polling loop is dead while you are
 *   inside the console.
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
#include "uart.h"
#include "c2n.h"
#include "onewire.h"

FIL     console_file;
FRESULT console_fres;
DIR     console_dir;

extern FIL     mountfile;

void dmatest(void);
void test_timing(void);
//void go_to_freeze_mode(void);

extern BOOL frozen;

#define LINESIZE 64

DWORD gethexvalue(char **line_in, BYTE offset)
{
    DWORD addr = 0L;
    char *line = &((*line_in)[offset]);
    BYTE b = 0;

	while(b < 8) {
		addr <<= 4;
		if((line[b] >= '0')&&(line[b] <= '9'))      addr |= (line[b] & 0x0F);
	    else if((line[b] >= 'A')&&(line[b] <= 'F')) addr |= (line[b] - 55);
	    else if((line[b] >= 'a')&&(line[b] <= 'f')) addr |= (line[b] - 87);
	    else break;
		b++;
	}
	addr >>= 4;
	*line_in = &line[b+1];

    return addr;
}

#define MAP1_WORD0 *((WORD*)MAP1_ADDR)
#define MAP1_WORD1 *((WORD*)MAP1_ADDR+2)
#define MAP1_WORD2 *((WORD*)MAP1_ADDR+4)

void console(void)
{
#ifdef DEVELOPMENT
    static char line[LINESIZE];
    static char tmp[40];
    static char *pline;
    static BYTE b;
//    static BYTE c,d;
	static WORD w, r;
//    static WORD t;
    static DWORD addr;
	static BYTE *mem;
	static BYTE *intmem;
//    static BYTE *hdr;
    static WORD maxmap;

    BYTE bdata;
            
    if(!uart_data_available())
        return;

	mem = (BYTE *)0x4000;
    tmp[NUM_COLMS] = 0;

    MAPPER_MAP3 = MAP3_IO;
    GPIO_IPOL   = BUTTONS | C64_IRQ; // buttons and stuff are active low
    GPIO_IMASK  = BUTTON1;           // turn on interrupt on middle button

    while(1) {
        printf("> ");
        memset(line, 0, LINESIZE);
		uart_read_buffer(line, LINESIZE);
		printf("\n");
        pline = &line[0];
        // snoop CR from end of line
        if(line[0])
            line[strlen(line)-1] = 0;

        if(strncmp(line, "erase", 5)==0) {
            flash_erase();
        }
/*
        if(strncmp(line, "dump", 4)==0) {
            dump_trace();
        }
*/
        if(strncmp(line, "flash", 5)==0) {
        	console_fres = f_open(&console_file, &line[6], FA_READ);
        	if(console_fres == FR_OK) {
        		printf("Do you want to program this file?\n");
        		uart_read_buffer( line, LINESIZE);

        		if(line[0] == 'y') {
        			flash_file(&console_file, 0);
        		}
        		f_close(&console_file);
        	} else {
        	    printf("File '%s' not found.\n", (char *)&line[6]);
        	}
        }
        if(strncmp(line, "exit", 4)==0)
            break;
        if(strncmp(line, "m ", 2)==0) {
			addr = gethexvalue(&pline, 2);
			printf("Printing from address %07lx\n", addr);

			w = (WORD)(addr >> 13); // calculate map
			MAPPER_MAP1 = w;

			w = (addr & 0x1FFF);
			for(b=0;b<128;b++) {
				if((b & 15)==0) {
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
        if(strncmp(line, "i ", 2)==0) {
			addr = gethexvalue(&pline, 2);
			printf("Printing internal address %04lx\n", addr);

            dump_hex((BYTE *)addr, 128);
        }
        if(strncmp(line, "poke ", 5)==0) {
            addr = gethexvalue(&pline, 5);
			w = (WORD)(addr >> 13); // calculate map
			MAPPER_MAP1 = w;
			w = (addr & 0x1FFF);
            b = (BYTE)gethexvalue(&pline, 0);
            printf("Writing %02x to %07lx. (w=%04x mem=%04x)\n", b, addr, w, mem);
            mem[w] = b;
        }
        if(strncmp(line, "oww ", 4)==0) {
            addr = gethexvalue(&pline, 4);
            bdata = (BYTE)gethexvalue(&pline, 0);
            printf("Writing Onewire value %02x to %03lx.\n", b, addr);
            onewire_progbuf((BYTE)addr, (char *)&bdata, 1);
        }
        if(strncmp(line, "owd", 3)==0) {
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
        if(strncmp(line, "o ", 2)==0) {
            addr = gethexvalue(&pline, 2);
            b = (BYTE)gethexvalue(&pline, 0);
			intmem = (BYTE *)addr;
            printf("Writing %02x to internal address %04lx.\n", b, addr);
            *intmem = b;
        }
        if(strncmp(line, "driveaddr ", 10)==0) {
            GPIO_SET2   = DRIVE_RESET;
            b = (BYTE)gethexvalue(&pline, 10);
            b &= 0x03;
            GPIO_CLEAR  = (0x03 << DRIVE_ADDR_BIT);
            GPIO_SET    = (b << DRIVE_ADDR_BIT);
            GPIO_CLEAR2 = DRIVE_RESET;
        }
        if(strncmp(line, "set ", 4)==0) {
            b = (BYTE)gethexvalue(&pline, 4);
            GPIO_SET2 = b;
        }
        if(strncmp(line, "clear ", 6)==0) {
            b = (BYTE)gethexvalue(&pline, 6);
            GPIO_CLEAR2 = b;
        }
        if(strncmp(line, "exit", 4)==0) {
            return;
        }
        if(strncmp(line, "sdram", 5)==0) {
            printf("Initializing sdram...");
            SDRAM_Init();
            printf(" done\n");
        }
        if(strncmp(line, "sdtest", 6)==0) {
            printf("Filling (and testing)... [Every dot is 128 kB]\n");
            for(w=4096;w<8192;w++) {
                r = TestPage(1, w);
                if(r) {
                    printf("Errors in page %4x (%d).\n", w, r);
                } else if((w % 16)==0) {
                    printf("."); 
                }
/*
                if((w & 0x1FF) == 0x1Ff) {
                    for(t=4096;t<w;t++) {
                        r = TestPage(0, t);
                        if(r) {
                            printf("X");
                        } else {
                            printf("v");
                        }
                    }
                    printf("\n");
                }
*/
            }
            printf("\nVerifying...\n");
            for(w=4096;w<8192;w++) {
                r = TestPage(0, w);
                if(r) {
                    printf("Errors in page %4x (%d).\n", w, r);
                } else if((w % 16)==0) {
                    printf("."); 
                }
            }
            printf("\n");
        }            
        if(strncmp(line, "ramtest ", 8)==0) {
            w = (WORD)gethexvalue(&pline, 8);
            w = TestPage(1, w);
            printf("%d errors.\n", w);
        }
        if(strncmp(line, "ramver ", 7)==0) {
            w = (WORD)gethexvalue(&pline, 7);
            w = TestPage(0, w);
            printf("%d errors.\n", w);

            if(w<10) {
                b = MAPPER_MAP1L ^ MAPPER_MAP1H;
                for(r=0;r<8192;r++) {
					if(*((BYTE*)MAP1_ADDR+r) != (b ^ (r & 0xFF))) {
					    printf("Expected: %02x. Got %02x. @ %04x\n", b ^ (r & 0xFF), *((BYTE*)MAP1_ADDR+r), r);
                    }
                    if((r & 0xFF) == 0xFF)
                        b--;
                }
            }
        }
        if(strncmp(line, "ramfill", 7)==0) {
            SDRAM_CTRL = 0x03;
            printf("Filling memory. Log disabled.\n");
//            for(w=4096;w<4352;w++) {
            for(w=6144;w<8192;w++) {
                FillPage(0xAA, w); // AA
            }
            SDRAM_CTRL = 0x07;
            printf("Filling complete. Log enabled.\n");
        }            
		if(strncmp(line, "ramfind " ,8)==0) {
			b = (BYTE)gethexvalue(&pline, 8);
			for(w=8064;w<8192;w++) { // 4096 - 4352
				MAPPER_MAP1 = w;
				for(r=0;r<8192;r++) {
					if(*((BYTE*)MAP1_ADDR+r) == b) {
						printf("Found @ %lx\n", (DWORD)((w - 4096) * 8192) + r);
					}
				}
			}
		}
        if(strncmp(line, "dump ", 5)==0) {
            maxmap = 7168; // 1024 pages
            MAPPER_MAP1 = maxmap-1;
            if(MAP1_WORD0 != 0xAAAA) {
                printf("Memory is not cleared before trace was enabled.\n");
                printf("Dumping just 16K.\n");
                maxmap = 6146;
            } 
            f_close(&console_file);
            console_fres = f_open(&console_file, &line[5], FA_CREATE_ALWAYS | FA_WRITE);
            if(console_fres == FR_OK) {
                for(w=6144;w<maxmap;w++) {
                    MAPPER_MAP1 = w;
                    if((MAP1_WORD0 != 0xAAAA) || (MAP1_WORD1 != 0xAAAA)) {
                        console_fres = f_write(&console_file, (void *)0x4000, 0x2000, &r);
                        printf("console_fres = %d | bytes written = %d\n", console_fres, r);
                    } else {
                        printf("%d pages written.\n", w-6144);
                        break;
                    }
                }
                f_close(&console_file);
            } else {
                printf("Can't open file for writing.\n");
            }
        }

        if(strncmp(line, "savepage ", 9)==0) {
            addr = gethexvalue(&pline, 9);
            MAPPER_MAP1 = (WORD)(addr >> 13);
            console_fres = f_open(&console_file, pline, FA_CREATE_ALWAYS | FA_WRITE);
            if(console_fres == FR_OK) {
                console_fres = f_write(&console_file, (void *)0x4000, 0x2000, &r);
                printf("console_fres = %d | bytes written = %d\n", console_fres, r);
                f_close(&console_file);
            } else {
                printf("Can't open file for writing (%s).\n", pline);
            }
        }

        if(strncmp(line, "load ", 4)==0) {
            f_close(&mountfile);
            console_fres = f_open(&mountfile, &line[5], FA_READ);
            if(console_fres == FR_OK) {
                ClearFloppyMem();
                load_d64(&mountfile, 0, 1);
            }
        }
/*
        if(strncmp(line, "play ", 5)==0) {
            f_close(&console_file);
            console_fres = f_open(&console_file, &line[5], FA_READ);
            if(console_fres == FR_OK) {
                r = 1;
                do {
                    if(!(C2N_STATUS & C2N_FIFO_ALMOSTFULL)) {
                        f_read(&console_file, (void *)C2N_FIFO_ADDR, 512, &r);
                        UART_DATA = '.';
                    } else {
                        C2N_CONTROL = C2N_ENABLE;
                    }
                } while(r);
            } else {
                printf("Opening file failed.\n");
            }
        }
*/
        if(strncmp(line, "save ", 5)==0) {
            console_fres = f_open(&console_file, &line[5], FA_WRITE | FA_CREATE_ALWAYS);
            if(console_fres == FR_OK) {

/*
                MAPPER_MAP1H = 0;
                for(b=0;b<35;b++) {
                    MAPPER_MAP1L = b;
                    w = find_trackstart();
                    if(!w) {
                        printf("Cannot find header for sector 0 on track %d.\n", b+1);
                    } else { // start found
                        d = get_num_sectors(b, 0);
                        for(c=0;c<d;c++) {
                            hdr = get_sector(b);
                            if(hdr[2] != c) {
                                printf("Wrong sector on track %d. Hdr=%02x Exp:%02x.\n", b+1, hdr[2], c);
                            }
                            if(hdr[3] != (b+1)) {
                                printf("Wrong track # on T/S %d/%d. Hdr=d.\n", b+1, c, hdr[3]);
                            }
                            f_write(&console_file, &sector_buf[1], 256, &w);
                            printf(".");
                        }
                        printf("\n");
                    }
                }
*/
                MAPPER_MAP1 = 0x1000;
                f_write(&console_file, (void *)0x4000, 0x2000, &w);
                f_close(&console_file);
            } else {
                printf("Can't open file for writing. Fres = %02x\n", console_fres);
            }
        }


    }
#endif
}

