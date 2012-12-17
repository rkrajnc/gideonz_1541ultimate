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
 *   Functions for loading / remembering the last disk that was mounted.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "sd.h"
#include "ff.h"
#include "uart.h"
#include "last.h"
#include "gcr.h"
#include "config.h"
#include "dump_hex.h"
#include "dir.h"

extern FIL 	  mountfile;
extern FIL    menu_file;
extern DWORD  FatFs_winsect;        // Current sector appearing in the win[] 

#define STATE_VERSION 0x01
#define STATE_KEY     0x5453414C
#define STATE_FILE    "last.41u"

struct t_last_state
{
    DWORD    key;
    BYTE     version;
    BYTE     valid;
    BYTE     pad[2];
    DWORD    state_sect; // to check if the file got moved
    DIRENTRY last_mount; // last D64 mounted
};

DWORD state_sector = 0;
struct t_last_state last_state;

static FRESULT res;
static WORD    numbytes;

void last_init(void)
{
    printf("Initializing State...\n");

    last_state.key     = STATE_KEY;
    last_state.version = STATE_VERSION;
    last_state.valid   = 0;
    last_state.pad[0]  = 0;
    last_state.pad[1]  = 0;
    memset(&last_state.last_mount, 0, sizeof(DIRENTRY));
    last_state.state_sect =  0;
    
}

void last_load(void)
{
    if(!cfg_get_value(CFG_LOAD_LAST))
        return;

    printf("Load State..\n");
    res = f_open(&menu_file, STATE_FILE, FA_READ);
    if(res == FR_OK) {
//        printf("Managed to .last file. Now reading.\n");
        res = f_read(&menu_file, &last_state, sizeof(struct t_last_state), &numbytes);
        state_sector = clust2sect(menu_file.org_clust);
        printf("Last: %d bytes read. Sector = %ld.\n", numbytes, state_sector);

        f_close(&menu_file);

        if(!numbytes) {
            last_init();
            return;
        }
        if(last_state.version != STATE_VERSION) {
            last_init();
            return;
        }
        if(last_state.key != STATE_KEY) {
            last_init();
            return;
        }
        if(last_state.state_sect != state_sector) {
            last_init();
            return;
        }
    } else {
        last_init();
        return;
    }
}    

void last_boot(void)
{
    BYTE wp;
    
    state_sector = 0;
    last_state.valid = 0;
    
    last_load();

    //printf("Boot State:\n");
    //dump_hex((BYTE *)&last_state, sizeof(struct t_last_state));

    if(last_state.last_mount.fcluster) {
        res = f_open_direct(&mountfile, &last_state.last_mount);
        if(res == FR_OK) {
            wp = (BYTE)(last_state.last_mount.fattrib & AM_RDO);
            if(last_state.last_mount.ftype == TYPE_D64) {
                load_d64(&mountfile, wp, 0); // no delay
            } else {
                load_g64(&mountfile, 1, 0);  // protected, no delay
            }
        }
    }
}

BOOL last_create(void)
{
    // try to create a .last file
    res = f_open(&menu_file, STATE_FILE, FA_WRITE | FA_CREATE_ALWAYS | FA_CREATE_HIDDEN);
    if(res == FR_OK) {
        res = f_write(&menu_file, &last_state, sizeof(struct t_last_state), &numbytes);
        state_sector = clust2sect(menu_file.org_clust);
        if(res == FR_OK) {
            state_sector = clust2sect(menu_file.org_clust);
            last_state.state_sect = state_sector;
        } else {
            last_state.valid = 0;
        }
        f_close(&menu_file);
        return TRUE;
    } else {
        last_state.valid = 0;
        return FALSE;
    }
}

void last_update(DIRENTRY *mount)
{
    if(!cfg_get_value(CFG_LOAD_LAST))
        return;

    memcpy(&last_state.last_mount, mount, sizeof(DIRENTRY));
//    dump_hex((BYTE *)&last_state, sizeof(struct t_last_state));

    if(!last_state.valid)
        if(!last_create())
            return;
    
    last_state.valid = 1;

    if(last_state.state_sect) {
        bank1_sd_writeSector(last_state.state_sect, (BYTE *)&last_state);
    } else {
        printf("Oops.. sector seems to be 0!\n");
    }

    printf("Last mount information written to sector %ld.\n", last_state.state_sect);   
}
