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
 *   This module implements the 1541 Ultimate menu on the C64 screen.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "version.h"
#include "data_tools.h"
#include "ff.h"
#include "mapper.h"
#include "gpio.h"
#include "keyb.h"
#include "screen.h"
#include "dir.h"
#include "c64_menu.h"
#include "c64_addr.h"
#include "copper.h"
#include "gcr.h"
#include "d64.h"
#include "t64.h"
#include "sid.h"
#include "freezer.h"
#include "copper.h"
#include "sd.h"
#include "diskio.h"
#include "flash.h"
#include "config.h"
#include "last.h"
#include "tap.h"

// local functions
static void handle_up(BYTE);
static void handle_down(BYTE);
static void dir_down(void);
static void dir_up(void);
static void enter_cbm(DIRENTRY *entry, BYTE mode);
static BOOL handle_select(BOOL);
static BYTE format_direntry_menu(WORD index, BYTE cbm);
static void perform_quick_seek(void);
static void reset_quick_seek(void);
static void handle_rename(void);

#define D64_EXT	".d64"
#define DMP_EXT ".dmp"

// shared stuff
extern FIL 	   	mountfile;
extern BOOL  	disk_change; // indicates that SD card was changed outside of the menu

FIL     menu_file;
DIR     menu_dir;
FRESULT menu_fres;

// the following variables are used by the copper routines to draw the bar!
WORD  menu_offset;  // offset index within directory
WORD  menu_index;   // selected entry

static BOOL  disk_changed;
static BOOL	 delete_always;
static WORD  num_entries;
static char  tmp[NUM_COLMS]; // temporary storage for dir line

BYTE  swap_delay;
BYTE  menu_dir_depth;
DWORD subdir_trace[MAX_DEPTH];
WORD  index_trace[MAX_DEPTH];
WORD  offset_trace[MAX_DEPTH];

static ENTRYNAME	quick_seek;
static char	 quick_seek_name[MAX_ENTRY_NAME];

static char  under_box[2][120];

DIRECTORY	menu_dir_cache; // Cache shared with main_loop. Keep non static.
static ENTRYNAME	menu_entry_name;

void   ClearFloppyMem(void);
//static char error_msg[60];

char default_msg[] =  "\004 ** 1541 Ultimate ** By Gideon Z **   \010\x32\003"
		                     "  Use cursor keys to navigate. "
		                     " F1 scrolls a page up;"
		                     " F7 scrolls a page down;"
		                     " Return selects an entry;"
		                     " DEL leaves a directory;"
		                     " Shift+DEL to delete an entry;"
		                     " Shift+'C' to create a new disk;"
		                     " Shift+'D' to create a new directory;"
		                     " Shift+'R' to rename;"
		                     " F3 to run a program;"
		                     " F2 to enter the setup menu."
		                     " You can type the entry name for quick search."
		                     " Use '?' for characters you are unsure of."
		                     " Enjoy!           ";

const BYTE my_copper[23] = { 0x12, 0x28, 0x16, 0x08,
                             0x12, 0x42, 0x20, 0x06, 0x21, 0x06,  // blue
                             0x12, 0x4A, 0x20, 0x00, 0x21, 0x00,  // black
                             0x12, 0xF2, 0x16, 0x00, 0x12, 0xFE, 0xFF }; 


extern BYTE extsout[];
BYTE scroll_enable;

static void handle_delete(void);

void menu_reset_global_scroll(void)
{
    // set global scroll parameters
    start_pos  = 81;
    width      = NUM_COLMS;
    num_lines  = NUM_LINES;
    do_color   = 0;
}

static BYTE menu_dir_change(DWORD cluster)
{
    static BYTE dot;

    dot = cfg_get_value(CFG_HIDE_DOT);
    return dir_change(cluster, &menu_dir_cache, dot);
}

void dump_mem_to_file(void)
{
    WORD w;
    WORD bytes_written;
    for(w=2048;w<2304;w++) {
        MAPPER_MAP1 = w;
        f_write(&menu_file, (void *)MAP1_ADDR, 8192, &bytes_written);
        printf("%d bytes written; addr = %06x\n", bytes_written, w << 13);
    } 
}

#define MAP1_WORD0 *((WORD*)MAP1_ADDR)
#define MAP1_WORD1 *((WORD*)MAP1_ADDR+2)
#define MAP1_WORD2 *((WORD*)MAP1_ADDR+4)

void dump_trace_to_file(void)
{
	WORD w, r;
	
    if(menu_fres == FR_OK) {
        for(w=4096;w<4352;w++) {
            MAPPER_MAP1 = w;
            if((MAP1_WORD0 != 0xAAAA) || (MAP1_WORD1 != 0xAAAA)) {
                menu_fres = f_write(&menu_file, (void *)0x4000, 0x2000, &r);
                printf("console_fres = %d | bytes written = %d\n", menu_fres, r);
            } else {
                printf("%d pages written.\n", w-4096);
                break;
            }
        }
        f_close(&menu_file);
    } else {
        printf("Can't open file for writing.\n");
    }
}

void check_change(void)
{
    if(card_change || disk_change) {
        card_change = 0; // I saw it! clear flag
        disk_change = 0;
        menu_init();
        disk_changed = TRUE;
		
        if(card_det) {
            printf("New card inserted! ");
            f_mountdrv();
            printf("Filesystem remounted.\n");
        	menu_fres = menu_dir_change(0L);
        	draw_directory();
        	last_load();
        } 
		else {
            f_close(&mountfile);
            if(cfg_get_value(CFG_CLR_ON_RMV)) {
                ClearFloppyMem();
            }
            clear_screen(0x0f);
            printf("Card removed!\n");
        }
    }
}

void set_title(void)
{
    char *scr = (char *)SCREEN1_ADDR;
    MAPPER_MAP1 = MAP_RAMDMA;

#ifdef DEVELOPMENT
    sprintf(&scr[1], " DEVELOPER VERSION! DO NOT DISTRIBUTE ");
#else
    switch(hardware_type) {
        case HW_PLUS:
            sprintf(&scr[1], "   ** 1541 Ultimate Plus - " SW_VERSION " **  ");
            break;
        case HW_ETHERNET:
            sprintf(&scr[1], " ** 1541 Ultimate Ethernet - " SW_VERSION " **");
            break;
        default:
            sprintf(&scr[1], "     ** 1541 Ultimate - " SW_VERSION " **     ");
    }
#endif
}

void draw_menu(void)
{
    char *scr = (char *)SCREEN1_ADDR;
    BYTE b;
    
    MAPPER_MAP1 = MAP_RAMDMA;

    set_title();
    for(b=40;b<80;b++)
        scr[b] = 2; // horizontal line
    for(b=0;b<40;b++)
        scr[(NUM_LINES+2)*40+b] = 2; // horizontal line
//    sprintf(&scr[920], "Space/Return = select | Cursors navigate");

    for(b=0;b<40;b++)
        COLOR_MAP3(b) = 1; // white title    

    // set global scroll parameters
	menu_reset_global_scroll();

    check_change();
    
    memcpy(copper_list, my_copper, 32);
    copper_pntr = 0;

	draw_directory();
	if(quick_seek.len != 0) {
		perform_quick_seek();
	}
	
    scroll_enable = cfg_get_value(CFG_SCROLL_EN);
    if(scroll_enable) {
    	scroll_msg  = default_msg;
    	new_message = 1;
    } else {
        memset(&scr[960], 32, 40); // clear out bottom line
    }
}    

void draw_directory(void)
{
    WORD idx, line_start;
    BYTE b;
    static char *scr;
    scr = (char *)(SCREEN1_ADDR);

	line_start = 81;
	
//    printf("Draw directory. Index=%d, offset=%d.\n", menu_index, menu_offset);
	// Offset corrections so we dont have to worry about menu_offset in other functions
	if((menu_offset == 0) && (menu_index >= NUM_LINES)) { // Is the offset zero and does the index never fit on the screen?
		menu_offset = (menu_index - (NUM_LINES / 2)); // Put the offset so that the menu_index lands in the middle
		if((menu_offset + NUM_LINES) > menu_dir_cache.num_entries) { // Does the offset fill the entire screen?
			menu_offset = menu_index - (NUM_LINES - 1); 	
		}
	}
//    printf("                Index=%d, offset=%d.\n", menu_index, menu_offset);
	
    idx = menu_offset;
    if((idx + NUM_LINES) > menu_dir_cache.num_entries) { // Does the offset fill the entire screen?
        if(menu_dir_cache.num_entries < NUM_LINES) {
            idx = 0; // Index x is zero because the dircache cannot fill the entire screen
        } else {
            idx = menu_dir_cache.num_entries - NUM_LINES;
        }
    }
        
    for(b = 0; b < NUM_LINES; b++, idx++) {
        format_direntry_menu(idx, menu_dir_cache.cbm_medium);
        MAPPER_MAP1 = MAP_RAMDMA;
        memcpy(scr + line_start, tmp, NUM_COLMS);
		start_pos = line_start;
		num_lines = 1;
		width = NUM_COLMS - 3; 
		set_color(0xF);
		start_pos = line_start + NUM_COLMS - 3;
		num_lines = 1;
		width = 3;
		set_color(0x7);
        line_start += 40;
    }
	
	menu_reset_global_scroll();

    copper_en   = 0x80;
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

BOOL string_box(WORD pos, char w, char *buffer, BYTE maxlen)
{
    BOOL ret;
    char b;
    char bar;
    static char *scr;
    static char *col;

	// Ignoring parameter w. In future the string_input schould scroll within this width
	w = maxlen + 2;
	
    MAPPER_MAP1 = MAP_RAMDMA;
    scr = (char *)(SCREEN1_ADDR + pos);
    col = (char *)(COLOR3_ADDR + pos);
	
    // make blue bar invisible
    bar = copper_list[COP_POS_BRDR]; // store color, in case we want to play with this
    copper_list[COP_POS_BRDR] = 0;
    copper_list[COP_POS_SCR]  = 0;

    //set_color(11); //dark grey DONT! You cannot restore this currently (or do the entire screen)

    // backup 3 lines
    for(b=0;b<120;b++) {
        under_box[0][b] = scr[b];
		under_box[1][b] = col[b];
    }
    
    // draw box from left to right
    scr[0] = 1; // upper left corner
    scr[40] = 4; // vertical bar
    scr[80] = 5; // lower left corner
    for(b=1;b<w-1;b++) {
        scr[b] = 2; // horizontal line
        scr[b+80] = 2;
        scr[b+40] = 20; // space
    }
    scr[w-1] = 3; // upper right corner
    scr[39+w] = 4; // vertical bar
    scr[79+w] = 6; // lower right corner

    // copy old text in box
    strncpy(scr+41, buffer, w-2);

    for(b=0;b<w;b++) {
        col[b]    = 0x0e;
        col[b+80] = 0x0e;		
		if((b > 0) && (b < (w-1))) {
			col[b+40] = 0x0F; // Grey
		}
		else {
			col[b+40] = 0x0e;
		}
    }
    
    ret = string_input(&scr[41], buffer, maxlen); // cursor on second line, second char
    
    // clean up
    for(b=0;b<120;b++) {
        scr[b] = under_box[0][b];
		col[b] = under_box[1][b];
    }

    copper_list[COP_POS_BRDR] = bar;
    copper_list[COP_POS_SCR]  = bar;

    return ret;    
}

BYTE question_box(WORD pos, char *options)
{
    BYTE ret;
    char b, w, len;
    char bar;
    static char *scr;
    static char *col;

	len = data_strnlen(options, NUM_COLMS);
	w = len + 2;
    MAPPER_MAP1 = MAP_RAMDMA;
    scr = (char *)(SCREEN1_ADDR + pos);
    col = (char *)(COLOR3_ADDR + pos);

    // make blue bar invisible
    bar = copper_list[COP_POS_BRDR]; // store color, in case we want to play with this
    copper_list[COP_POS_BRDR] = 0;
    copper_list[COP_POS_SCR]  = 0;

    //set_color(11); //dark grey DONT! You cannot restore this currently (or do the entire screen)

    // backup 3 lines
    for(b=0;b<120;b++) {
        under_box[0][b] = scr[b];
		under_box[1][b] = col[b];
    }
    // draw box from left to right
    scr[0] = 1; // upper left corner
    scr[40] = 4; // vertical bar
    scr[80] = 5; // lower left corner
    for(b=1;b<w-1;b++) {
        scr[b] = 2; // horizontal line
        scr[b+80] = 2;
        scr[b+40] = options[b-1]; // space
    }
    scr[w-1] = 3; // upper right corner
    scr[39+w] = 4; // vertical bar
    scr[79+w] = 6; // lower right corner

    for(b=0;b<w;b++) {
        col[b]    = 0x0e;
        col[b+80] = 0x0e;
		if((b > 0) && (b <= len)) {
			if((options[b-1] >= 'a') && (options[b-1] <= 'z')) // Actually I mean caps
				col[b+40] = 0x0F; // Grey
			else
	        	col[b+40] = 0x01; // White
		}
		else {
			col[b+40] = 0x0e;
		}
    }

	do {
       ret = kb_buf_getch();
	} while(!ret);

	ret |= 0x20;
	
    // clean up
    for(b=0;b<120;b++) {
        scr[b] = under_box[0][b];
		col[b] = under_box[1][b];
    }

    copper_list[COP_POS_BRDR] = bar;
    copper_list[COP_POS_SCR]  = bar;

    return ret;    	
}

static
void perform_quick_seek(void)
{
	DIRENTRY *foundentry;

	foundentry = dir_find_entry(&quick_seek, NULL, &menu_dir_cache);
	if(foundentry != NULL) {
		menu_entry_name.name_str = foundentry->fname;
		menu_entry_name.len = data_strnlen(menu_entry_name.name_str, MAX_ENTRY_NAME);
		menu_index = dir_getindex(&menu_dir_cache, &menu_entry_name);
		menu_offset = 0;
		draw_directory();
		if(quick_seek.len != 0) {
			// the highlighted entry will be the last indicated by offset and index and the number of entries
			start_pos = 81 + ((menu_index - menu_offset) * 40);
			width = quick_seek.len;
			num_lines = 1;
			set_color(0x1);
			menu_reset_global_scroll();
		}
	}
	else { // no yield. Strip one character
		quick_seek.len--;
		quick_seek_name[quick_seek.len] = 0;
	}
}

static
void reset_quick_seek(void)
{
	if(quick_seek.len != 0) {
		draw_directory();
		quick_seek.len = 0;
		quick_seek.name_str[0] = 0;
	}
}

void menu_init(void)
{
	menu_dir_cache.dir_address = MENU_DIR_ADDR;
    menu_dir_depth  = 0;
    menu_offset = 0;
    menu_index  = 0;
    num_entries = 0;
	quick_seek.command_len = 0;
	quick_seek.command_str = NULL;
	quick_seek.illegal = FALSE;
	quick_seek.len = 0;
	quick_seek.name_str = quick_seek_name;
	quick_seek.pattern = FALSE;
	quick_seek.replace = FALSE;
	quick_seek.unit = 0;
	quick_seek.wildcard = TRUE; // ALWAYS OTHERWISE IT WOULD NOT BE A SEEK NOW WOULD IT?
}

BOOL menu_run(BYTE boot)
{
    BYTE key, len;
    BOOL run;
    BOOL flash_flag;
    
    printf("Running menu...\n");

    check_change();
	disk_changed = FALSE;

	delete_always = FALSE;
	
    draw_menu();
    GPIO_IMASK = C64_IRQ; // turn on IRQ from VIC

    run = TRUE;
    flash_flag = FALSE;

    swap_delay = (BYTE)cfg_get_value(CFG_SWAP_DELAY);
    
    while(run) {
        check_change();

        if(disk_changed && boot && !card_det) {  // SD-remove => exit
            run = FALSE;
            exit_freeze();
            break;
        }

        key = kb_buf_getch();
        if(!key)
            continue;

        if((key == '!')&&(flash_flag)) {
            handle_flash();
            continue;
        }
        flash_flag = FALSE;


		len = quick_seek.len;
		if( ( 
				(!((key < 32)||(key > 127))) ||
				((key == 0x14) && (len > 0))   // backspace (inst/del)
    		)
			&& // legal but unwanted
			(
				!((key >= 'A') && (key <= 'Z'))  // CAPS for special functions?
    		)
    		&&
    		    (key != 0x2F)   // filter /, since we need it for root
		) {
			if(key == 0x14) {
				len--;
			}
			else {
				if((key >= 'a') && (key <= 'z')) {
					key &= ~0x20;
				}
				quick_seek_name[len] = key;
				len++;
			}
			
			quick_seek_name[len] = 0;
			quick_seek.len = len;

			printf("seek: %s", quick_seek.name_str);
			perform_quick_seek();
			continue;
		}
		
        switch(key) {
	        case 'F':  // prefix
	            flash_flag = TRUE;
	            break;
	        case 0x03: // runstop
	            run = FALSE;
	            exit_freeze();
	            break;
	        case 0x11: // down
	        	reset_quick_seek();
	            handle_down(1);
	            break;
	        case 0x91: // up
	        	reset_quick_seek();
	            handle_up(1);
	            break;
	        case 0x85: // F1 -> page up
	        	reset_quick_seek();
	            handle_up(NUM_LINES-1);
	            break;
	        case 0x89: // F2 -> setup
                if(!run_config()) {
                    set_title();
                    draw_directory();
					if(quick_seek.len != 0) {
						perform_quick_seek();
					}
                    scroll_msg = default_msg;
                    new_message = SCROLL_OUT;
                } else {
       	        	reset_quick_seek();
                    clear_screen(0x0f);
                    disk_change = 1;
                    draw_menu();
                }
                swap_delay = (BYTE)cfg_get_value(CFG_SWAP_DELAY);
                break;
	        case 0x86: // F3 -> RUN
	            run = handle_select(TRUE);
	            break;

	        case 0x88: // F7 -> page down
   	        	reset_quick_seek();
	            handle_down(NUM_LINES-1);
	            break;
	        case 0x1D: // right
   	        	reset_quick_seek();
	            dir_down();
	            break;
	        case 0x9D: // left
	        case 0x14: // backspace (inst/del)
//	        case 0x60: // <-
				reset_quick_seek();
	            dir_up();
	            break;
	        case 0x2F: // '/'
   	        	reset_quick_seek();
	            menu_dir_depth = 1;
	            dir_up();
	            break;
	        case 'Q': //0x7E:
	            handle_dump_mem();
	            break; 
	        case 'D': 
   	        	reset_quick_seek();
	            handle_create_dir();
	            break;
	        case 'R': 
   	        	reset_quick_seek();
	            handle_rename();
	            break;
/*	        case 'S': 
	            handle_create_disk(FALSE);
	            break; 								OBSOLETE */
	        case 'C':
   	        	reset_quick_seek();			
//	            handle_create_disk(TRUE);
	            handle_create_disk();
	            break; 
	        case 0x0D: // return
   	        	reset_quick_seek();
	            run = handle_select(FALSE);
	            break;
			case 0x94:
   	        	reset_quick_seek();
				handle_delete();
				break;
	        default:
	            printf("Unknown character. %02x.\n", key);
        }
    }
    return disk_changed;
}


static
void handle_down(BYTE a)
{
    static char *scr;
    
    if(!menu_dir_cache.num_entries)
        return;

    if(menu_index+a < menu_dir_cache.num_entries)
        menu_index+=a;
    else
        menu_index = menu_dir_cache.num_entries - 1;
        
    if(menu_index >= menu_offset + NUM_LINES) { // scroll
        if(a == 1) {
            menu_offset ++;
            scroll_down();
            scr = (char *)(SCREEN1_ADDR + 41 + NUM_LINES * 40);
            format_direntry_menu(menu_index, menu_dir_cache.cbm_medium);
            MAPPER_MAP1 = MAP_RAMDMA;
            memcpy(scr, tmp, NUM_COLMS);
        } 
		else {
            menu_offset = 1 + menu_index - NUM_LINES;
            draw_directory();
        }
    }
}

static
void handle_up(BYTE a)
{
    static char *scr;

    if(!menu_dir_cache.num_entries)
        return;

    // would we go above the top?
    if(menu_index >= a)
        menu_index -= a;
    else
        menu_index = 0;

	if(menu_index < menu_dir_cache.info_fields) {
		menu_index = menu_dir_cache.info_fields;
	}
	
    if(menu_offset > menu_index) { // scroll
        menu_offset = menu_index;
        if(a == 1) {
            scroll_up();
            scr = (char *)(SCREEN1_ADDR + 81);
            format_direntry_menu(menu_index, menu_dir_cache.cbm_medium);
            MAPPER_MAP1 = MAP_RAMDMA;
            memcpy(scr, tmp, NUM_COLMS);
        } 
		else {
            draw_directory();
        }
    }
}

static
void enter_cbm(DIRENTRY *entry, BYTE mode)
{
	BYTE res;
    if(menu_dir_depth+1 < MAX_DEPTH) {
        index_trace[menu_dir_depth] = menu_index;
        offset_trace[menu_dir_depth] = menu_offset;
        subdir_trace[menu_dir_depth++] = entry->fcluster;
        f_open_direct(&menu_file, entry);
        switch(mode) {
            case TYPE_T64:
    			res = t64_dir(&menu_file, &menu_dir_cache, TRUE);
                break;
            case TYPE_D64:
            case TYPE_D71:
            case TYPE_D81:
    			res = d64_dir(&menu_file, &menu_dir_cache, TRUE, mode);
    		    break;
    		case TYPE_SID:
    		    res = sid_dir(&menu_file, &menu_dir_cache);
    		    break;
    		default:
    		    res = FR_OK+1; // not ok
		}
        if(res == FR_OK)  {
            menu_index = menu_dir_cache.info_fields;
			menu_offset = 0;
            draw_directory();
			
        } 
		else {
            menu_dir_depth--;
        }
    }
}

static
void dir_down(void)
{
    static DIRENTRY *entry;
    
    entry = dir_getentry(menu_index, &menu_dir_cache);
    switch(entry->ftype) {
	    case TYPE_DIR:
	        handle_select(FALSE);
	        break;
	    case TYPE_D64:
		case TYPE_D71:
	    case TYPE_D81:			
	    case TYPE_T64:			
        case TYPE_SID:
            enter_cbm(entry, entry->ftype);
            break;
	    default:
	        printf("Can't enter this type.\n");   
    }
}

static
void dir_up(void)
{
    if(menu_dir_depth == 0)
        return;
        
    menu_dir_depth--;
	if(menu_dir_cache.cbm_medium) {
		f_close(&menu_file);	// ale a cbm medium implies the file pointer is used
	}
    if(menu_dir_depth == 0)
        menu_dir_change(0L);
    else
        menu_dir_change(subdir_trace[menu_dir_depth-1]);
    
    menu_index = index_trace[menu_dir_depth];
    menu_offset = offset_trace[menu_dir_depth];

    draw_directory();
}    

static
BOOL handle_select(BOOL runflag)
{
    static DIRENTRY *entry;
    static DWORD start;
    static WORD  c64_strt, c64_len;
    BYTE wp, tr;
    
    entry = dir_getentry(menu_index, &menu_dir_cache);
    switch(entry->ftype) {
    case TYPE_DIR:
        if(menu_dir_depth+1 < MAX_DEPTH) {
            index_trace[menu_dir_depth] = menu_index;
            offset_trace[menu_dir_depth] = menu_offset;
            subdir_trace[menu_dir_depth++] = entry->fcluster;
            menu_dir_change(entry->fcluster);
            menu_index = menu_offset = 0;
            draw_directory();
        }
        break;
		
    case TYPE_GCR:
        f_close(&mountfile);
        last_update(entry); // should be done before we do exit_freeze, due to mapper
        f_open_direct(&mountfile, entry);
        wp = (BYTE)(entry->fattrib & AM_RDO);
        exit_freeze();
        load_g64(&mountfile, wp, swap_delay);
        return FALSE;

    case TYPE_D64:
        f_close(&mountfile);
        last_update(entry); // should be done before we do exit_freeze, due to mapper
        f_open_direct(&mountfile, entry);
        wp = (BYTE)(entry->fattrib & AM_RDO);
        exit_freeze();
        tr = load_d64(&mountfile, wp, swap_delay);
        printf("Number of tracks read: %d\n", tr);
        return FALSE;
		
    case TYPE_T64:
        f_open_direct(&menu_file, entry);
        exit_freeze();
        t64_loadfirst(&menu_file, runflag);
        return FALSE;
		
    case TYPE_PRG:
        f_open_direct(&menu_file, entry);
        exit_freeze();
        prg_loadfile(runflag);
        return FALSE;
		
	case TYPE_TAP:
		if(tap_open_file(entry)) {
			exit_freeze();
			return FALSE;
		}
		break;

    case TYPE_SID:
        f_open_direct(&menu_file, entry);
        exit_freeze();
        sid_load(&menu_file, 0xFF);
        return FALSE;

    case TYPE_TUNE: // file is already open
        wp = (BYTE)entry->fcluster;
        exit_freeze();
        sid_load(&menu_file, wp);
        return FALSE;

    case TYPE_PRGD64:
		{
			DIRENTRY tmpentry;
			/* Needed for f_open_direct */
			tmpentry.fcluster = menu_file.org_clust;
			tmpentry.fsize = menu_file.fsize;
			tmpentry.fattrib = (menu_file.flag & FA_WRITE) ? 0 : AM_RDO; // Write protect the D64
	        if(runflag) { // run, so mount the disk, too
	            scroll_msg = "\003Mounting disk... ";
	            new_message = SCROLL_OUT;
	            f_close(&mountfile);
	            f_open_direct(&mountfile, &tmpentry);
	            wp = (BYTE)(entry->fattrib & AM_RDO); // this should work, since all d64 entries inherit the readonly flag (TODO!)
	            load_d64(&mountfile, wp, swap_delay);
                last_update(&tmpentry);
	        }
    	}
		// FALLTROUGH       
    case TYPE_PRGD71:
    case TYPE_PRGD81:
        exit_freeze();
        MAPPER_MAP1 = MENU_DIR_ADDR; // so we can read entry members again
        d64_loadfile(&menu_file, entry->fcluster, runflag, entry->ftype);
        return FALSE;        

    case TYPE_PRGT64:
        start = entry->fcluster;
        c64_strt = *(WORD *)&entry->fname[20];
        c64_len  = (WORD)entry->fsize;
        exit_freeze();
        t64_loadfile(&menu_file, start, c64_strt, c64_len, runflag);
        return FALSE;        
    default:
        printf("Unknown type.\n");
    }
    return TRUE;
}

void handle_flash(void)
{
    static DIRENTRY *entry;
    
    entry = dir_getentry(menu_index, &menu_dir_cache);
    if(entry->ftype != TYPE_HEX) {
        scroll_msg  = "\004 Selected entry is not a valid for flashing!   \011";
        new_message = SCROLL_OUT;
        return;
    }

    menu_fres = f_open_direct(&menu_file, entry);
    if(menu_fres == FR_OK) {
        scroll_msg = "\002Flashing... Do not turn off power!   ";
        new_message = SCROLL_OUT;
        GPIO_CLEAR2 = SOUND_ENABLE;
    	if(!flash_file(&menu_file, 0)) {
            scroll_msg  = "\004    ERROR: Flashing NOT successful!!    \010\x32   \011";
            new_message = SCROLL_OUT;
    	} else {
            scroll_msg = default_msg;
            new_message = SCROLL_OUT;
        }
        GPIO_SET2 = SOUND_ENABLE;
	    f_close(&menu_file);
	}
}

//void handle_create_disk(BOOL blank)
void handle_create_disk(void)
{
    char filename[32];
//    BOOL pass;
    DWORD dir_cluster;
    
//    if(blank)
        scroll_msg = "\003Enter name of new disk...     \001 ";
//    else
//        scroll_msg = "\003Enter filename to save disk to...  \001 ";

    new_message = SCROLL_OUT;

    filename[0] = '\0';
    
    do {
        if(!string_box(446, NR_OF_EL(filename) - sizeof(D64_EXT),
						filename, NR_OF_EL(filename) - sizeof(D64_EXT)))
            break;
        
        strcat(filename, D64_EXT);
		dir_analyze_name(filename, NR_OF_EL(filename), &menu_entry_name);
		if( menu_entry_name.pattern ||
			menu_entry_name.command_len != 0 || // THIS IS ALLOWED BUT NOT YET IMPLEMENTED (for replace will point to @)
			menu_entry_name.unit != 0 ||
			menu_entry_name.wildcard || 
			menu_entry_name.illegal) {

            scroll_msg = " Illegal name. Try again.  ";
            new_message = SCROLL_OUT;
		}
		else  {
//            if(blank)
                scroll_msg = " Creating disk. Please wait... ";
//            else
//                scroll_msg = " Saving disk. Please wait... ";
            new_message = SCROLL_OUT;

            if(menu_dir_depth) {
                dir_cluster = subdir_trace[menu_dir_depth-1];
            } else {
                dir_cluster = 0L;
            }
            f_opendir_direct(&menu_dir, dir_cluster);
			// DONT CALL ANY FUNCTION AFTER OPEN DIR DIRECT THAT TOUCHES THE MENU DIR OBJECT
			// OR ELSE THE CREATE FILE ENTRY IS NOT ABLE TO CHECK IF THE FILE EXISTS
            menu_fres = f_create_file_entry(&menu_file, &menu_dir, menu_entry_name.name_str, D64_SIZE, menu_entry_name.replace, FALSE);
            if(menu_fres) {
				if(menu_fres == FR_EXISTS) {
                	scroll_msg = "\005  Object already exists!   \011";
				}
				else {
        	        scroll_msg = "\005  Error creating file!   \011";
				}
           	    new_message = SCROLL_OUT;
                return;
            }
//            if(blank) {
                menu_fres = d64_create(&menu_file, menu_entry_name.name_str);
//                printf("(menu) result = %d\n", menu_fres);
                f_close(&menu_file);
//            } else {
//                pass = d64_save(&menu_file, error_msg) == FR_OK;
//               f_close(&menu_file);
//                if(!pass) {
//                    scroll_msg = error_msg;
//                    new_message = SCROLL_OUT;
//                    continue;
//                }
//			}
            menu_dir_change(dir_cluster);
			menu_index = dir_getindex(&menu_dir_cache, &menu_entry_name);
			menu_offset = 0;
			draw_directory();
            break;
        }
    } while(1);

    scroll_msg = default_msg;
    new_message = SCROLL_OUT;
}

void handle_dump_mem(void)
{
    char filename[32];
    DWORD dir_cluster;
    
    scroll_msg = "\003Enter filename for mem dump...   \001 ";
    new_message = SCROLL_OUT;

    filename[0] = '\0';
    do {
        if(!string_box(446,  NR_OF_EL(filename) - sizeof(DMP_EXT),
							filename, NR_OF_EL(filename) - sizeof(DMP_EXT)))
            break;
        
        strcat(filename, DMP_EXT);
		dir_analyze_name(filename, NR_OF_EL(filename), &menu_entry_name);
		if( menu_entry_name.pattern ||
			menu_entry_name.command_len != 0 || // THIS IS ALLOWED BUT NOT YET IMPLEMENTED
			menu_entry_name.unit != 0 ||
			menu_entry_name.wildcard || 
			menu_entry_name.illegal) {

            scroll_msg = " Illegal name. Try again ";
            new_message = SCROLL_OUT;
		}
		else {
            scroll_msg = " Dumping memory to file... ";
            new_message = SCROLL_OUT;

            if(menu_dir_depth) {
                dir_cluster = subdir_trace[menu_dir_depth-1];
            } else {
                dir_cluster = 0L;
            }
            f_opendir_direct(&menu_dir, dir_cluster);
            menu_fres = f_create_file_entry(&menu_file, &menu_dir, menu_entry_name.name_str, 0L, menu_entry_name.replace, FALSE);
			if(menu_fres) {
				if(menu_fres == FR_EXISTS) {
	                	scroll_msg = "\005  Object already exists!   \010\x32\003\011";
				}
				else {
        	        scroll_msg = "\005  Error creating file!   \010\x32\003\011";
				}
           	    new_message = SCROLL_OUT;
                return;
            }
			dump_mem_to_file();
            f_close(&menu_file);
            menu_dir_change(dir_cluster);
            draw_directory();
            break;
        }
    } while(1);

    scroll_msg = default_msg;
    new_message = SCROLL_OUT;
}

void handle_create_dir(void)
{
    char dirname[24];
    DWORD dir_cluster;
    
    scroll_msg = "\003Enter name of new directory... \001 ";
    new_message = SCROLL_OUT;

    dirname[0] = '\0';
    do {
        if(!string_box(448, NR_OF_EL(dirname), 
							dirname, NR_OF_EL(dirname)))
            break;
        
		dir_analyze_name(dirname, NR_OF_EL(dirname), &menu_entry_name);
		if( menu_entry_name.pattern ||
			menu_entry_name.command_len != 0 || // THIS IS ALLOWED BUT NOT YET IMPLEMENTED 
			menu_entry_name.unit != 0 ||
			menu_entry_name.wildcard || 
			menu_entry_name.illegal) {

            scroll_msg = " Illegal name. Try again ";
            new_message = SCROLL_OUT;
		}
		else {
            scroll_msg = " Creating directory. Please wait... ";
            new_message = SCROLL_OUT;

            if(menu_dir_depth) {
                dir_cluster = subdir_trace[menu_dir_depth-1];
            } else {
                dir_cluster = 0L;
            }
            f_opendir_direct(&menu_dir, dir_cluster); 
			// DONT CALL ANY FUNCTION AFTER OPEN DIR DIRECT THAT TOUCHES THE MENU DIR OBJECT
			// OR ELSE THE CREATE DIRECTORY IS NOT ABLE TO CHECK IF THE FILE EXISTS
            menu_fres = f_create_dir_entry(&menu_dir, menu_entry_name.name_str, menu_entry_name.replace);
            if(menu_fres) {
                printf("result = %d\n", menu_fres);
				if(menu_fres == FR_EXISTS) {
                	scroll_msg = "\005  Object already exists!   \010\x32\003\011";
				}
				else {
	                scroll_msg = "\005  Error creating file!   \010\x32\003\011";
				}
                new_message = SCROLL_OUT;
                return;
            } else {                
                menu_dir_change(dir_cluster);
				menu_index = dir_getindex(&menu_dir_cache, &menu_entry_name);
				menu_offset = 0;
				draw_directory();
                break;
            }
        }
    } while(1);

    scroll_msg = default_msg;
    new_message = SCROLL_OUT;
}


static
void handle_delete(void)
{
    static DIRENTRY *entry;
    DWORD dir_cluster;

	BYTE sure = 'y';

	if((menu_dir_cache.num_entries == 0) || 
		(menu_dir_cache.cbm_medium && (menu_dir_cache.num_entries == 1))) {
		return;
	}

	if(!delete_always) {
	    scroll_msg = "\003ARE YOU SURE?            \001 ";
    	new_message = SCROLL_OUT;
		sure = question_box(451," Yes, No, Always ");
		if(sure == 'a') {
			sure = 'y';
			delete_always = TRUE;
		}
	    scroll_msg = default_msg;
	}

	if(sure == 'y') {	
		if(menu_dir_depth) {
		    dir_cluster = subdir_trace[menu_dir_depth-1];
		} else {
		    dir_cluster = 0L;
		}
		entry = dir_getentry(menu_index, &menu_dir_cache);
		f_opendir_direct(&menu_dir, dir_cluster);
		menu_fres = f_unlink_direct(&menu_dir, entry);
		if(menu_fres == FR_OK) {
		    menu_dir_change(dir_cluster);
		    draw_directory();
		}
		else {
			 scroll_msg = "\005  Error deleting entry!   \011";	
		}
	}
	
    new_message = SCROLL_OUT;
}

static
void handle_rename(void)
{
    static DIRENTRY *entry;
    DWORD dir_cluster;
    char new_name[40];

	if((menu_dir_cache.num_entries == 0) || 
		(menu_dir_cache.cbm_medium && (menu_dir_cache.num_entries == 1))) {
		return;
	}

    // copy old name to change...
	entry = dir_getentry(menu_index, &menu_dir_cache);
    strncpy(new_name, entry->fname, MAX_ENTRY_NAME);
    new_name[MAX_ENTRY_NAME] = '\0';
    
    if(!string_box(440, 38, new_name, 38))
        return;

	if(menu_dir_depth) {
	    dir_cluster = subdir_trace[menu_dir_depth-1];
	} else {
	    dir_cluster = 0L;
	}
    //printf("Dir cluster: %ld\n", dir_cluster);
	entry = dir_getentry(menu_index, &menu_dir_cache);
	f_opendir_direct(&menu_dir, dir_cluster);
	menu_fres = f_rename_direct(entry, &menu_dir, new_name);

	if(menu_fres == FR_OK) {
	    menu_dir_change(dir_cluster);
	    draw_directory();
	}
	else {
		 scroll_msg = "\005  Error renaming!   \011";	
         new_message = SCROLL_OUT;
	}
}

static
BYTE format_direntry_menu(WORD index, BYTE cbm)
{
    static DIRENTRY *entry;
    static BYTE typ;
    static WORD size;    
    static BYTE b,c;
    
    entry = dir_getentry(index, &menu_dir_cache);
    if(!entry) {
        memset(tmp, 32, NUM_COLMS);
        return TYPE_UNKNOWN;
    }

    typ   = entry->ftype;
    
    for(c=1,b=0;b<MAX_ENTRY_NAME;b++) {
        if(c)
            c = entry->fname[b];
        if(c)
            tmp[b] = c;
        else
            tmp[b] = ' ';
    }
    for(;b<35;b++)
        tmp[b] = ' ';
        
    memcpy(&tmp[35], &extsout[typ*3], 3);

    if(entry->fattrib & AM_RDO)
        tmp[33] = 'R';
        
    switch(cbm) {
    case 1: // d64
        size = (WORD)entry->fsize;
        break;
    case 2: // t64
        size = (WORD)((entry->fsize + 253)/254);
        break;
    default: // generic 
        size  = (WORD)((entry->fsize + 1023) >> 10);
        break;
    }
    
    if(typ != TYPE_DIR)
        sprintf(&tmp[24], cbm?"%4u blks":"%6ukB", size);

    tmp[32] = ' ';
    
    return typ;
}

void prg_loadfile(BOOL run)
{
    static WORD i,m,n,e;
    static WORD bytes_read;

    dma_load_init();

    // load file into C64 memory
    menu_fres = f_read(&menu_file, &i, 2, &bytes_read);

    printf("Start address of .PRG = %04x.\n", i);
    e = i;
    m = (i >> 13) | MAP_RAMDMA;
    i &= 0x1FFF;
    n = 0x2000- i;
    i |= MAP1_ADDR;
    
    while(bytes_read) {
        MAPPER_MAP1 = m;
        f_read(&menu_file, (void *)i, n, &bytes_read);
        printf("%d bytes read to addr %04x. Requested %04x bytes.\n", bytes_read, i, n);
        e += bytes_read;
        i = MAP1_ADDR;
        n = 0x2000;
        m ++;
    }

    // send end address of load
    MAPPER_MAP1 = MAP_RAMDMA;
    *(WORD *)(MAP1_ADDR + 0x002D) = e;

    dma_load_exit(run?DMA_RUN:DMA_BASIC);
}
