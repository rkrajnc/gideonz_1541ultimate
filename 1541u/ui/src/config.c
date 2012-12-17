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
 *   This module implements functions to store/retrieve configuration items
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ff.h"
#include "mapper.h"
#include "gpio.h"
#include "copper.h"
#include "flash.h"
#include "config.h"
#include "mem_tools.h"
#include "dump_hex.h"
#include "onewire.h"

// Configurable Items

const char *en_dis[] = { "Disabled", "Enabled" };
//const char *on_off[] = { "Off", "On" };
const char *yes_no[] = { "No", "Yes" };
//const char *cart_mode[] = { "None", "Final Cart III", "Action Replay", "RAM Exp Unit", "Standard 8K", "Standard 16K" };
const char *pal_ntsc[] = { "PAL", "NTSC" };

const char *cart_mode[] = { "None",
                            "Final Cart III",
                            "Action Replay V4.2 PAL",
                            "Action Replay V6.0 PAL",
                            "Retro Replay V3.8p PAL",
                            "SuperSnapshot V5.22 PAL",
                            "TAsm / CodeNet PAL",
                            "Action Replay V5.0 NTSC",
                            "Retro Replay V3.8a NTSC",
                            "SuperSnapshot V5.22 NTSC",
                            "TAsm / CodeNet NTSC",
                            "Epyx Fastloader",
                            "Custom 8K ROM",
                            "Custom 16K ROM",
                            "Custom System 3 ROM",
                            "Custom Ocean V1 ROM",
                            "Custom Ocean V2/T2 ROM",
                            "Custom Final III ROM",
                            "Custom Retro Replay ROM",
                            "Custom Snappy ROM",
                         };

const char *reu_size[] = { "128 KB", "256 KB", "512 KB", "1 MB", "2 MB", "4 MB", "8 MB", "16 MB" };    
const char *rom_sel[] = { "CBM 1541", "1541 C", "1541-II", "Load from SD" };

BYTE *cart_type;
BYTE *cart_base;

/*
BYTE cart_types[7]       = { 0x80, 0x04, 0x07, 0x07, 0x86, 0x85, 0x86 }; // bit 7 says if we can enable REU or not

BYTE cart_bases_pal[7]   = { 0x00, 0x02, 0x04, 0x05, 0x08, 0x0A, 0x12 };
BYTE cart_bases_ntsc[7]  = { 0x00, 0x02, 0x0C, 0x05, 0x0E, 0x10, 0x0E }; // AR6 should be 0x0D
*/
//                    none, final,ar4,  ar6,  RR,   SS,   TASM, AR5,  RR,   SS,   TASM, Epyx, 8K,   16K,  Sys3, Ocean,O.T2,Final,Retro,Snappy
BYTE cart_types[] = { 0xC0, 0x04, 0x07, 0x07, 0xC6, 0x85, 0xC6, 0x07, 0xC6, 0x85, 0xC6, 0x0A, 0xC1, 0xC2, 0x88, 0x89, 0x8B, 0x04, 0xC6, 0x85 };
BYTE cart_bases[] = { 0x00, 0x04, 0x02, 0x03, 0x08, 0x0A, 0x12, 0x0C, 0x0E, 0x10, 0x14, 0x16, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 };

WORD drv_rom_loc[4] = { 0x081E, 0x0834, 0x0836, 0x0000 };

// writable strings
char boot_file[32];
char owner_name[32];
char cust_cart[32];

const struct t_cfg_definition cfg_items[] = {
    { 1, CFG_OWNER_NAME, "Owner",                   "%s", NULL,               CFG_TYPE_STRING, 1, 31, (WORD)owner_name, owner_name },
    { 1, CFG_BOOT_FILE,  "Application to boot",     "%s", NULL,               CFG_TYPE_STRING, 1, 31, (WORD)"appl.bin", boot_file },
//    { 0, CFG_PAL_NTSC,   "Video System",            "%s", (char **)pal_ntsc,  CFG_TYPE_ENUM,   0,  1, 0, NULL },
    { 1, CFG_1541_ENABLE,"1541 Drive",              "%s", (char **)en_dis,    CFG_TYPE_ENUM,   0,  1, 1, NULL },
    { 1, CFG_1541_ADDR,  "1541 Drive Bus ID",       "%d", NULL,               CFG_TYPE_BYTE,   8, 11, 8, NULL },
    { 1, CFG_1541_ROM,   "1541 ROM Select",         "%s", (char **)rom_sel,   CFG_TYPE_ENUM,   0,  3, 2, NULL },
    { 2, CFG_1541_RAMBO, "1541 RAMBOard",           "%s", (char **)en_dis,    CFG_TYPE_ENUM,   0,  1, 0, NULL },
    { 1, CFG_LOAD_LAST,  "Load last mounted disk",  "%s", (char **)yes_no,    CFG_TYPE_ENUM,   0,  1, 0, NULL },
    { 1, CFG_SWAP_DELAY, "1541 Disk swap delay",    "%d00 ms", NULL,          CFG_TYPE_BYTE,   1, 10, 1, NULL },
    { 1, CFG_IEC_ENABLE, "IEC SDCard I/F",          "%s", (char **)en_dis,    CFG_TYPE_ENUM,   0,  1, 1, NULL },
    { 1, CFG_IEC_ADDR,   "IEC SDCard I/F Bus ID",   "%d", NULL,               CFG_TYPE_BYTE,   8, 30, 9, NULL },
    { 1, CFG_CART_MODE,  "Cartridge",               "%s", (char **)cart_mode, CFG_TYPE_ENUM,   0, 19, 4, NULL },
    { 2, CFG_CUST_CART,  "Custom Cart ROM",         "%s", NULL,               CFG_TYPE_STRING, 1, 31, "myrom.bin", cust_cart },
    { 2, CFG_REU_ENABLE, "RAM Expansion Unit",      "%s", (char **)en_dis,    CFG_TYPE_ENUM,   0,  1, 0, NULL },
    { 2, CFG_REU_SIZE,   "REU Size",                "%s", (char **)reu_size,  CFG_TYPE_ENUM,   0,  7, 4, NULL },
    { 1, CFG_CLR_ON_RMV, "Pull SD = remove floppy", "%s", (char **)en_dis,    CFG_TYPE_ENUM,   0,  1, 1, NULL },
    { 1, CFG_SWAP_BTN,   "Swap reset/freeze btns",  "%s", (char **)yes_no,    CFG_TYPE_ENUM,   0,  1, 0, NULL },
    { 1, CFG_HIDE_DOT,   "Hide '.'-files",          "%s", (char **)yes_no,    CFG_TYPE_ENUM,   0,  1, 1, NULL },
    { 1, CFG_SCROLL_EN,  "Scroller in menu",        "%s", (char **)en_dis,    CFG_TYPE_ENUM,   0,  1, 1, NULL },
    { 1, CFG_MENU_START, "Startup in menu",         "%s", (char **)yes_no,    CFG_TYPE_ENUM,   0,  1, 0, NULL },
    { 4, CFG_ETH_ENABLE, "Ethernet Interface",      "%s", (char **)en_dis,    CFG_TYPE_ENUM,   0,  1, 0, NULL }         
};

WORD  cfg_entries = 0;

// local definitions
struct t_cfg_flash_entry
{
    BYTE    id;
    BYTE    type;
    BYTE    length;
    BYTE    value[];
};

static struct t_cfg_ram_entry *config_in_ram;

//**EXPORTED STRUCTURE**//
struct t_cfg_ram_entry config_entries_ram[NR_OF_EL(cfg_items)];

void cfg_load_defaults(BYTE mask)
{
    BYTE b,i;
    static const struct t_cfg_definition *def;
    
    for(b=0,i=0;b<NR_OF_EL(cfg_items);b++) {
        def = &cfg_items[b];
        if(def->enabled & mask) {
            config_entries_ram[i].def   = def;
            config_entries_ram[i].value = def->def;  // definition -> default
            if(def->type == CFG_TYPE_STRING) {
                config_entries_ram[i].string = (char *)def->def;
            } else {
                config_entries_ram[i].string = NULL;
            }
            i++;
        }
    }
    cfg_entries = i;
}

void cfg_read_owner(void)
{
    BYTE b;
    for(b=0;b<16;b++) {
        owner_name[b] = (char)onewire_readmem(b);
    }
    owner_name[16]=0;
    printf("Thank you, %s!\n", owner_name);
}

void cfg_load_flash(void)
{
    BYTE *pnt, b;
    static struct t_cfg_flash_entry *entry;

    MAPPER_MAP1 = flash_get_cfg_page();

    pnt = (BYTE *)MAP1_ADDR;    

    do {
        entry = (struct t_cfg_flash_entry *)pnt;
        //printf("Entry = %02x. pnt = %p. Checking against %d ram entries\n", entry->id, pnt, cfg_entries);
        if(entry->id != 0xFF) { // not end
            for(b=0;b<cfg_entries;b++) {
                config_in_ram = &config_entries_ram[b];
                if(config_in_ram->def->id == entry->id) { // found!
                    if(config_in_ram->def->type != entry->type) {
                        printf("Error! Entry type mismatch!!\n");
                        break;
                    }
                    switch(entry->type) {
                    case CFG_TYPE_WORD:
                        config_in_ram->value = *((WORD *)&entry->value[0]);
                        break;
                    case CFG_TYPE_BYTE:
                    case CFG_TYPE_ENUM:
                        config_in_ram->value = (WORD)entry->value[0];
                        // if the value in flash is outside of what we support, return to our default.
                        if((config_in_ram->value < config_in_ram->def->min) || 
                           (config_in_ram->value > config_in_ram->def->max))  {
                            config_in_ram->value = config_in_ram->def->def;
                        }
                        break;
                    case CFG_TYPE_STRING:
                        strncpy(config_in_ram->def->string_store, (char *)&entry->value[0], entry->length);
                        config_in_ram->string = config_in_ram->def->string_store;
                        break;
                    default:
                        printf("Error: unknown type, reading flash configuration.\n");
                    }
                    break;
                }
            }
            pnt += (3 + entry->length);
        }
    } while(entry->id != 0xFF);
}

void cfg_save_flash(void)
{
    // now we have to encode our ram entries into flash entries, whereby
    // saving the entries that do not have a match. Therefore, we just
    // create a new page in SRAM, fill it with FF, and copy all entries
    // from flash that are not on our list, and add items from our list.
    // Then, at last erase sector in flash and write page.

    BYTE *spnt, *dpnt, b;
    BOOL found;
    static struct t_cfg_flash_entry *sentry;
    static struct t_cfg_flash_entry *dentry;
    
    FillPage(0xFF, MENU_DIR_ADDR); // reuse! This function also sets MAP1
    MAPPER_MAP2 = flash_get_cfg_page();
    spnt = (BYTE *)MAP2_ADDR;    
    dpnt = (BYTE *)MAP1_ADDR;
        
    // first copy all the flash entries that are not in our ram list
    do {
        sentry = (struct t_cfg_flash_entry *)spnt;
        dentry = (struct t_cfg_flash_entry *)dpnt;

        if(sentry->id != 0xFF) { // not end
            for(found=FALSE,b=0;b<cfg_entries;b++) {
                config_in_ram = &config_entries_ram[b];
                if(config_in_ram->def->id == sentry->id) { // found!
                    found=TRUE;
                    break;
                }
            }
            if(!found) { // not found; copy old entry
//                printf("entry not found with id %d. raw copy. spnt=%p dpnt=%p\n", sentry->id, spnt, dpnt);
                memcpy(dentry, sentry, 3+sentry->length);
                dpnt += (3 + dentry->length);
            }
        }
        spnt += (3 + sentry->length);
    } while(sentry->id != 0xFF);

    // now add all entries from ram
    for(b=0;b<cfg_entries;b++) {
        config_in_ram = &config_entries_ram[b];
        dentry = (struct t_cfg_flash_entry *)dpnt;
        //printf("inserting entry with id %d. dpnt=%p\n", ram->def->id, dpnt);
        dentry->id     = config_in_ram->def->id;
        dentry->type   = config_in_ram->def->type;
        switch(config_in_ram->def->type) {
        case CFG_TYPE_BYTE:
        case CFG_TYPE_ENUM:
            dentry->length = 1;
            dentry->value[0] = (BYTE)config_in_ram->value;
            break;
        case CFG_TYPE_WORD:
            dentry->length = 2;
            *((WORD *)(&dentry->value[0])) = config_in_ram->value;
            break;
        case CFG_TYPE_STRING:
            dentry->length = strlen(config_in_ram->string)+1;
            strcpy((char *)&dentry->value[0], config_in_ram->string);
            break;
        }
        dpnt += (3 + dentry->length);
    }        
//    dump_hex((BYTE *)MAP1_ADDR, 192);
    
    GPIO_CLEAR2 = SOUND_ENABLE;
    flash_erase_block(flash_get_cfg_sector());
    flash_page(MENU_DIR_ADDR, flash_get_cfg_page());
    GPIO_SET2 = SOUND_ENABLE;
}

struct t_cfg_ram_entry *cfg_get_entry(BYTE id)
{
    BYTE b;
    
    for(b=0;b<cfg_entries;b++) {
        config_in_ram = &config_entries_ram[b];
        if(config_in_ram->def->id == id)
            return config_in_ram;
    }
    return NULL;
}

WORD cfg_get_value(BYTE id)
{
    cfg_get_entry(id);
    return config_in_ram->value;
}    

char *cfg_get_string(BYTE id)
{
    cfg_get_entry(id);
    if(config_in_ram->def->type != CFG_TYPE_STRING)
        return "Item is not a string.";
        
    return config_in_ram->string;
}    
