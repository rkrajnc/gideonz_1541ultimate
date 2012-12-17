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
 *   This module implements the user interface to the configuration items
 *   (the configuration screen)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ff.h"
#include "mapper.h"
#include "gpio.h"
#include "keyb.h"
#include "screen.h"
#include "c64_menu.h"
#include "c64_addr.h"
#include "copper.h"
#include "flash.h"
#include "config.h"
#include "mem_tools.h"
#include "dump_hex.h"
#include "version.h"

// local functions
static void cfg_handle_up(void);
static void cfg_handle_down(void);
static void cfg_handle_right(void);
static void cfg_handle_left(void);
static void cfg_handle_select(void);
static BYTE format_cfg_entry(int index);

// the following variables are used by the copper routines to draw the bar!
extern WORD  menu_offset;  // offset index within directory
extern WORD  menu_index;   // selected entry
extern WORD  cfg_entries;
extern struct t_cfg_ram_entry config_entries_ram[];

static struct t_cfg_ram_entry *config_in_ram;

static char  tmp[NUM_COLMS]; // temporary storage for cfg line

static
char config_msg[] = "\004   ** 1541 Ultimate Setup Screen **    \010\x32\003"
                    "  Use Up/Down to select configurable item. Use Left/Right cursor keys "
                    "to change item. Shift+'S' to save. Pressing Run Stop returns to file browser.         ";

void set_cfg_title(void)
{
    char *scr = (char *)SCREEN1_ADDR;
    MAPPER_MAP1 = MAP_RAMDMA;
    sprintf(&scr[1], " ** 1541 Ultimate - Configuration ** ");
}

void draw_config(void)
{
    WORD idx, line_start;
    BYTE b, entryval_start;
    static char *scr;
    scr = (char *)(SCREEN1_ADDR);
    
    set_cfg_title();
    clear_screen(0x07);

	line_start = 81; // 2 lines down + 1 right
		
    idx = menu_offset;
    if((menu_offset + NUM_LINES) > cfg_entries)
        idx = 0;

    for(b=0;b<NUM_LINES;b++,idx++) {

		entryval_start = format_cfg_entry(idx);
        start_pos = line_start + (WORD)entryval_start + 1;
        MAPPER_MAP1 = MAP_RAMDMA;
        memcpy(scr + line_start, tmp, NUM_COLMS);
        line_start += 40;
        num_lines = 1;
		width = (line_start - start_pos) - 2;
		set_color(0x1);

    }
	
	menu_reset_global_scroll();

    copper_en   = 0x80;
}

BOOL run_config(void)
{
    BYTE key;
    BYTE bar;
    BOOL run;
    WORD backup_index;
    WORD backup_offset;
    BOOL flash_flag;
    
    printf("Running config screen...\n");

    // change color of scroll bar
    bar = copper_list[COP_POS_BRDR]; // store color, in case we want to play with this
    copper_list[COP_POS_BRDR] = 2; // switch to red
    copper_list[COP_POS_SCR]  = 2;

    backup_index = menu_index;
    backup_offset = menu_offset;
    menu_index = 0;
    menu_offset = 0;
    draw_config();
    scroll_msg  = config_msg;
    new_message = SCROLL_OUT;

    run = TRUE;
    flash_flag = FALSE;
    
    while(run) {
        key = kb_buf_getch();
        if(!key)
            continue;
/*
        if((key == '!')&&(flash_flag)) {
            handle_flash();
            continue;
        }
        flash_flag = FALSE;
*/
        switch(key) {
	        case 'S':  // prefix
	            flash_flag = TRUE;
	            break;
	        case 0x03: // runstop
	            run = FALSE;
	            break;
	        case 0x11: // down
	            cfg_handle_down();
	            break;
	        case 0x91: // up
	            cfg_handle_up();
	            break;
	        case 0x1D: // right
	            cfg_handle_right();
	            break;
	        case 0x9D: // left
	            cfg_handle_left();
	            break;
	        case 0x0D: // return
	        case 0x20: // space
	            cfg_handle_select();
	            break;
            case 0x93: // clear
                //flash_erase_block(flash_get_cfg_sector());
                cfg_load_defaults(hardware_mask);
                draw_config();
                break;
	        default:
	            printf("Unknown character. %02x.\n", key);
        }
    }
    if(flash_flag)
        cfg_save_flash();

    if(cfg_get_value(CFG_1541_ENABLE)) {
        GPIO_CLEAR2 = DRIVE_RESET;
    } else {
        GPIO_SET2 = DRIVE_RESET;
    }
    
    copper_list[COP_POS_BRDR] = bar;
    copper_list[COP_POS_SCR]  = bar;
    menu_index = backup_index;
    menu_offset = backup_offset;

    return flash_flag; // when flashed, the directory is overwritten, menu should reread the dir
}

static
void cfg_handle_down(void)
{
    static char *scr;
    
    if(!cfg_entries)
        return;

    if(menu_index+1 < cfg_entries)
        menu_index+=1;
    else
        menu_index = cfg_entries - 1;
        
    if(menu_index >= menu_offset + NUM_LINES) { // scroll
        menu_offset ++;
        scroll_down();
        scr = (char *)(SCREEN1_ADDR + 41 + NUM_LINES * 40);
        format_cfg_entry(menu_index);
        MAPPER_MAP1 = MAP_RAMDMA;
        memcpy(scr, tmp, NUM_COLMS);
    }
}

static
void cfg_handle_up(void)
{
    static char *scr;

    if(!cfg_entries)
        return;

    if(menu_index >= 1)
        menu_index -= 1;
    else
        menu_index = 0;
    
    if(menu_offset > menu_index) { // scroll
        menu_offset = menu_index;
        scroll_up();
        scr = (char *)(SCREEN1_ADDR + 81);
        format_cfg_entry(menu_index);
        MAPPER_MAP1 = MAP_RAMDMA;
        memcpy(scr, tmp, NUM_COLMS);
    } 
	
	menu_reset_global_scroll();

}

static void draw_cfg_entry(void)
{
	WORD line_start, entryval_start;	
    char *scr;

	line_start = 81 + ((menu_index - menu_offset) * 40);
	
    scr = (char *)(SCREEN1_ADDR + line_start);

    entryval_start = format_cfg_entry(menu_index);
    memcpy(scr, tmp, NUM_COLMS);
	start_pos = line_start;
    num_lines = 1;	
	width = (BYTE)(entryval_start - 1);
	set_color(0x7);
	start_pos = line_start + entryval_start + 1;
    num_lines = 1;
	width = 38 - width;
    set_color(0x1);	

	menu_reset_global_scroll();
}    

static
void cfg_handle_left(void)
{
    config_in_ram = &config_entries_ram[menu_index];

    if(config_in_ram->def->type == CFG_TYPE_STRING)
        return; // only right pops up a string input box

    if(config_in_ram->value)
        config_in_ram->value --; // left
    else
        config_in_ram->value = config_in_ram->def->max; // wrap

    // check boundaries
    if(config_in_ram->value < config_in_ram->def->min)
        config_in_ram->value = config_in_ram->def->max; // wrap
    else if(config_in_ram->value > config_in_ram->def->max)
        config_in_ram->value = config_in_ram->def->max;
    
    // redraw
    draw_cfg_entry();
}    

static
void cfg_handle_right(void)
{
    config_in_ram = &config_entries_ram[menu_index];

    if(config_in_ram->def->type == CFG_TYPE_STRING) {
        cfg_handle_select();
        return;
    }

    config_in_ram->value ++; // right
    // check boundaries
    if(config_in_ram->value < config_in_ram->def->min)
        config_in_ram->value = config_in_ram->def->min;
    else if(config_in_ram->value > config_in_ram->def->max)
        config_in_ram->value = config_in_ram->def->min;  // wrap
    
    // redraw
    draw_cfg_entry();
}

static
void cfg_handle_select(void)
{
    config_in_ram = &config_entries_ram[menu_index];
    
    if(config_in_ram->def->type != CFG_TYPE_STRING)
        return;
        
    if(!string_box(420-(config_in_ram->def->max/2), config_in_ram->def->max, config_in_ram->def->string_store, config_in_ram->def->max))
        return;

    // set pointer to custom string, but don't store empty strings
    if(strlen(config_in_ram->def->string_store))
        config_in_ram->string = config_in_ram->def->string_store;
    
    // redraw
    draw_cfg_entry();
}

static
BYTE format_cfg_entry(int index)
{
    static const struct t_cfg_definition *def;
    static char buf[32];
    static BYTE b, len, d;

    memset(tmp, ' ', NR_OF_EL(tmp));
    if(index >= cfg_entries)
        return 0;
            
    config_in_ram = &config_entries_ram[index];
    def = config_in_ram->def;

    // definition entry found; construct line with current option
    sprintf(tmp, def->item_text);

    switch(def->type) {
        case CFG_TYPE_BYTE:
        case CFG_TYPE_WORD:
            sprintf(buf, def->item_format, config_in_ram->value);
            break;
        case CFG_TYPE_ENUM:
            sprintf(buf, def->item_format, def->items[config_in_ram->value]);
            break;
        case CFG_TYPE_STRING:
            sprintf(buf, def->item_format, config_in_ram->string);
            break;
        default:
            sprintf(buf, "Unknown type.");
    }
    len = strlen(buf);
    // right align copy
    for(b = len, d=NR_OF_EL(tmp)-1; b; b--)
        tmp[d--] = buf[b-1];

	return d;	
}
