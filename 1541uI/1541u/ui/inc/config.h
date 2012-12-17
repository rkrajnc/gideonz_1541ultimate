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
 *   Definitions of the existing configuration items, structs and functions.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"


#define CFG_END_OF_LIST 0xFF
#define CFG_BOOT_FILE   0x00
#define CFG_1541_ADDR   0x01
#define CFG_IEC_ENABLE  0x02
#define CFG_IEC_ADDR    0x03
#define CFG_CART_MODE   0x04
#define CFG_REU_SIZE    0x05
#define CFG_1541_ENABLE 0x06
#define CFG_1541_ALTROM 0x07
#define CFG_CLR_ON_RMV  0x08
#define CFG_OWNER_NAME  0x09
#define CFG_SWAP_BTN    0x0A
#define CFG_HIDE_DOT    0x0B
#define CFG_SCROLL_EN   0x0C
#define CFG_PAL_NTSC    0x0D
#define CFG_MENU_START  0x0E
#define CFG_1541_ROM    0x0F
//#define CFG_RESUME      0x10  // debug values
//#define CFG_SID_VOLUME  0x11  // debug values
#define CFG_ETH_ENABLE  0x12
#define CFG_REU_ENABLE  0x13
#define CFG_1541_RAMBO  0x14
#define CFG_LOAD_LAST   0x15
#define CFG_SWAP_DELAY  0x16
#define CFG_CUST_CART   0x17

#define CFG_TYPE_BYTE   0x01
#define CFG_TYPE_WORD   0x02
#define CFG_TYPE_ENUM   0x03
#define CFG_TYPE_STRING 0x04


struct t_cfg_definition
{
    BYTE enabled;
    BYTE id;
    char *item_text;
    char *item_format;
    char **items;
    BYTE type;
    WORD min, max, def;
    char *string_store;
};

struct t_cfg_ram_entry
{
    const struct  t_cfg_definition *def;
    WORD    value;
    char    *string;
};

BOOL   run_config(void);

struct t_cfg_ram_entry *cfg_get_entry(BYTE id);

WORD  cfg_get_value(BYTE id);
char *cfg_get_string(BYTE id);
void cfg_load_defaults(BYTE mask);
void cfg_load_flash(void);
void cfg_save_flash(void);
void cfg_read_owner(void);

extern WORD drv_rom_loc[4];
extern char owner_name[];
#endif
