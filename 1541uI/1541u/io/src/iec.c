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
 *    Abstract:
 *
 *	Provides functions for an additional IEC drive interfacing to the SD-CARD
 *	directly. THIS IS NOT 15xx compatible but should be used from basic.
 *
 *	BIG NOTE: THIS SHOULD LAND IN THE MANUAL (if one is ever written)
 *				THE SD CARD I/F IS NOT COMPATIBLE WITH ANY COMMODORE DRIVE
 *				THEREFORE IT SHOULD NOT BE USED AS DEVICE 8 OR 9 AS THE COMMODORE
 *				WILL ATTEMPT TO UPLOAD AND EXECUTE CODE.
 *				THEN IT EXPECTS US TO JABBER SOME PROTOCOL WHICH WE HAVE NO INTENTION
 *				TO IMPLEMENT.....SO KEEP IT BASIC (AND SLOW)
 */
#include "manifest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#include "gpio.h"
#include "ff.h"
#include "uart.h"
#include "gcr.h"
#include "spi.h"
#include "mem_tools.h"
#include "mapper.h"
#include "dir.h"
#include "asciipetscii.h"
#include "data_tools.h"
#include "d64.h"
#include "t64.h"
#include "iec.h"
#include "last.h"

extern BYTE card_det;
extern BYTE extsout[];
extern FIL  mountfile;

DIR     iec_dir;
FILINFO iec_finfo;
FRESULT iec_fres;
FIL     iec_file;

#define MAX_DEPTH    12
#define MAX_NAME_LEN 16

BYTE  	iec_dir_depth;
WORD 	iec_dir_index;
DWORD 	iec_subdir_trace[MAX_DEPTH];
extern WORD sal_dir_index; // unfortunate crossing of module boundary!

BOOL eoi;
BYTE io, ioprev;
BYTE times[8];
BOOL listen, talk, talking, shutup;
BYTE last_error;
BYTE hex[16] = "0123456789ABCDEF";

typedef struct {
	BYTE nr;
	CHAR* msg;
	BYTE len;
} IEC_ERROR_MSG;

// Error,ErrorText(,Track,Sector\015) 
// The track and sector will be send seperately

const CHAR msg00[] = " OK";						//00
const CHAR msg01[] = "FILES SCRATCHED";			//01	Track number shows how many files were removed
const CHAR msg20[] = "READ ERROR"; 				//20 (Block Header Not Found)
//const CHAR msg21[] = "READ ERROR"; 				//21 (No Sync Character)
//const CHAR msg22[] = "READ ERROR"; 				//22 (Data Block not Present)
//const CHAR msg23[] = "READ ERROR"; 				//23 (Checksum Error in Data Block)
//const CHAR msg24[] = "READ ERROR"; 				//24 (Byte Decoding Error)
const CHAR msg25[] = "WRITE ERROR";				//25 (Write/Verify Error)
const CHAR msg26[] = "WRITE PROTECT ON";		//26
//const CHAR msg27[] = "READ ERROR"; 				//27 (Checksum Error in Header)
//const CHAR msg28[] = "WRITE ERROR"; 			//28 (Long Data Block)
const CHAR msg29[] = "DISK ID MISMATCH";		//29
const CHAR msg30[] = "SYNTAX ERROR";			//30 (General)
const CHAR msg31[] = "SYNTAX ERROR";			//31 (Invalid Command)
const CHAR msg32[] = "SYNTAX ERROR";			//32 (Command Line > 58 Characters)
const CHAR msg33[] = "SYNTAX ERROR";			//33 (Invalid Filename)
const CHAR msg34[] = "SYNTAX ERROR";			//34 (No File Given)
const CHAR msg39[] = "SYNTAX ERROR";			//39 (Invalid Command)
const CHAR msg50[] = "RECORD NOT PRESENT";		//50
const CHAR msg51[] = "OVERFLOW IN RECORD";		//51
//const CHAR msg52[] = "FILE TOO LARGE";			//52
const CHAR msg60[] = "WRITE FILE OPEN";			//60
const CHAR msg61[] = "FILE NOT OPEN";			//61
const CHAR msg62[] = "FILE NOT FOUND";			//62
const CHAR msg63[] = "FILE EXISTS";				//63
const CHAR msg64[] = "FILE TYPE MISMATCH";		//64
//const CHAR msg65[] = "NO BLOCK";				//65
//const CHAR msg66[] = "ILLEGAL TRACK AND SECTOR";//66
//const CHAR msg67[] = "ILLEGAL SYSTEM T OR S";	//67
const CHAR msg70[] = "NO CHANNEL";	            //70
const CHAR msg71[] = "DIRECTORY ERROR";			//71
const CHAR msg72[] = "DISK FULL";				//72
const CHAR msg73[] = "SD CARD I/F DOS V1.1";	//73 DOS MISMATCH(Returns DOS Version)
const CHAR msg74[] = "DRIVE NOT READY";			//74

const CHAR msg_c1[] = "BAD COMMAND";			//custom
const CHAR msg_c2[] = "UNIMPLEMENTED";			//custom

const char *ext_per_type[] = { ".prg", ".seq", ".usr", ".rel" };

#define ERR_OK							00
#define ERR_FILES_SCRATCHED				01
#define ERR_READ_ERROR					20
#define ERR_WRITE_ERROR					25
#define ERR_WRITE_PROTECT_ON			26
#define ERR_DISK_ID_MISMATCH			29
#define ERR_SYNTAX_ERROR_GEN			30
#define ERR_SYNTAX_ERROR_CMD			31
#define ERR_SYNTAX_ERROR_CMDLENGTH		32
#define ERR_SYNTAX_ERROR_NAME			33
#define ERR_SYNTAX_ERROR_NONAME			34
#define ERR_SYNTAX_ERROR_CMD15			39
#define ERR_RECORD_NOT_PRESENT          50
#define ERR_OVERFLOW_IN_RECORD          51
#define ERR_WRITE_FILE_OPEN				60
#define ERR_FILE_NOT_OPEN				61
#define ERR_FILE_NOT_FOUND				62
#define ERR_FILE_EXISTS					63
#define ERR_FILE_TYPE_MISMATCH			64
#define ERR_NO_CHANNEL          		70
#define ERR_DIRECTORY_ERROR				71
#define ERR_DISK_FULL					72
#define ERR_DOS							73
#define ERR_DRIVE_NOT_READY				74
// custom

#define ERR_BAD_COMMAND					75
#define ERR_UNIMPLEMENTED				76


// generic
#define MODE_IDLE						0x00

// io channels
#define MODE_DIR_OPEN					0x01
#define MODE_DIR_READ					0x02
#define MODE_PRG_READ					0x03
#define MODE_D64PRG_READ				0x04
#define MODE_T64PRG_ADDR				0x05
#define MODE_T64PRG_READ				0x06
#define MODE_REL_READ                   0x07

// dos commands
#define MODE_MEM_READ					0x80

#define MODE_ERROR						0xFF

// access types
#define ACC_READ    0x01
#define ACC_WRITE   0x02
#define ACC_APPEND  0x04


const IEC_ERROR_MSG last_error_msgs[] = {
		{ 00,(CHAR*)msg00,NR_OF_EL(msg00) - 1 },
		{ 01,(CHAR*)msg01,NR_OF_EL(msg01) - 1 },
		{ 20,(CHAR*)msg20,NR_OF_EL(msg20) - 1 },
//		{ 21,(CHAR*)msg21,NR_OF_EL(msg21) - 1 },
//		{ 22,(CHAR*)msg22,NR_OF_EL(msg22) - 1 },
//		{ 23,(CHAR*)msg23,NR_OF_EL(msg23) - 1 },
//		{ 24,(CHAR*)msg24,NR_OF_EL(msg24) - 1 },
		{ 25,(CHAR*)msg25,NR_OF_EL(msg25) - 1 },
		{ 26,(CHAR*)msg26,NR_OF_EL(msg26) - 1 },
//		{ 27,(CHAR*)msg27,NR_OF_EL(msg27) - 1 },
//		{ 28,(CHAR*)msg28,NR_OF_EL(msg28) - 1 },
		{ 29,(CHAR*)msg29,NR_OF_EL(msg29) - 1 },
		{ 30,(CHAR*)msg30,NR_OF_EL(msg30) - 1 },
		{ 31,(CHAR*)msg31,NR_OF_EL(msg31) - 1 },
		{ 32,(CHAR*)msg32,NR_OF_EL(msg32) - 1 },
		{ 33,(CHAR*)msg33,NR_OF_EL(msg33) - 1 },
		{ 34,(CHAR*)msg34,NR_OF_EL(msg34) - 1 },
		{ 39,(CHAR*)msg39,NR_OF_EL(msg39) - 1 },
		{ 50,(CHAR*)msg50,NR_OF_EL(msg50) - 1 },
		{ 51,(CHAR*)msg51,NR_OF_EL(msg51) - 1 },
//		{ 52,(CHAR*)msg52,NR_OF_EL(msg52) - 1 },
		{ 60,(CHAR*)msg60,NR_OF_EL(msg60) - 1 },
		{ 61,(CHAR*)msg61,NR_OF_EL(msg61) - 1 },
		{ 62,(CHAR*)msg62,NR_OF_EL(msg62) - 1 },
		{ 63,(CHAR*)msg63,NR_OF_EL(msg63) - 1 },
		{ 64,(CHAR*)msg64,NR_OF_EL(msg64) - 1 },
//		{ 65,(CHAR*)msg65,NR_OF_EL(msg65) - 1 },
//		{ 66,(CHAR*)msg66,NR_OF_EL(msg66) - 1 },
//		{ 67,(CHAR*)msg67,NR_OF_EL(msg67) - 1 },
		{ 70,(CHAR*)msg70,NR_OF_EL(msg70) - 1 },
		{ 71,(CHAR*)msg71,NR_OF_EL(msg71) - 1 },
		{ 72,(CHAR*)msg72,NR_OF_EL(msg72) - 1 },
		{ 73,(CHAR*)msg73,NR_OF_EL(msg73) - 1 },
		{ 74,(CHAR*)msg74,NR_OF_EL(msg74) - 1 },

		{ 75,(CHAR*)msg_c1,NR_OF_EL(msg_c1) - 1 },
		{ 76,(CHAR*)msg_c2,NR_OF_EL(msg_c2) - 1 }
};


// ASSUMPTION the defintions in ff.h must map the array position!!!!!
const BYTE FF_TO_IEC[] = {
		ERR_OK,						// FR_OK,						all is ok
		ERR_DRIVE_NOT_READY,		// FR_NOT_READY,				sdcard not initialized
		ERR_FILE_NOT_FOUND,  		// FR_NO_FILE,					depends on the context. e.g. fopen with no file 			
		ERR_FILE_NOT_FOUND,  		// FR_NO_PATH,					path cannot be traced
		ERR_SYNTAX_ERROR_NAME, 		// FR_INVALID_NAME,				wrong name	
		ERR_FILE_NOT_OPEN, 			// FR_DENIED,					could not open for some reason		
		ERR_DISK_FULL, 				// FR_DISK_FULL,				disk full
		ERR_READ_ERROR, 			// FR_RW_ERROR,					sadly we cannot distinguish		
		ERR_DIRECTORY_ERROR, 		// FR_INCORRECT_DISK_CHANGE, 	pulled out an sd while we had files open
		ERR_WRITE_PROTECT_ON, 		// FR_WRITE_PROTECTED,			that little switch is set on lock
		ERR_DRIVE_NOT_READY, 		// FR_NOT_ENABLED,				this cannot be generated by ff.c but is defined
		ERR_DRIVE_NOT_READY,		// FR_NO_FILESYSTEM,			the inserted card has no filesystem we know			
		ERR_FILE_EXISTS				// FR_EXISTS,					the file exists
};


#define CMD_TYPE_SPECIAL 0
#define CMD_TYPE_DOS	 1

typedef struct _CMDS {
	CHAR *cmdStr;
	BYTE type;
	void (*proc)(void);
} CMDS;

static void cmd_automount (void);
static void cmd_memread (void);
static void cmd_kill (void);
static void cmd_kill_emulated (void);

static void cmd_initialize (void);
static void cmd_initialize_emulated (void);
static void cmd_changeaddr (void);
static void cmd_changeaddr_emulated (void);
static void cmd_settingsave (void);

static void cmd_mount_d64 (void);
static void cmd_change_dir (void);
static void cmd_scratch (void);
static void cmd_rename (void);
static void cmd_copy (void);
static void cmd_makedir (void);
static void cmd_maked64 (void);
static void cmd_position (void);

static struct t_cfg_ram_entry *config_in_ram;

void handle_dos_cmd();

// use caps only for petscii <> ascii compat without converting
static CMDS commands[] = {
	//{ "M-R", 		CMD_TYPE_SPECIAL, cmd_memread }, --> SEE FAST COMMANDS FOR DETAILS (will not work as dos command)
	{ "AUTOMOUNT", 	CMD_TYPE_SPECIAL, cmd_automount },		
	{ "CD", 		CMD_TYPE_DOS, cmd_change_dir },
	{ "COPY",		CMD_TYPE_DOS, cmd_copy },
	{ "INIT0",		CMD_TYPE_SPECIAL, cmd_initialize },
	{ "INIT1",		CMD_TYPE_SPECIAL, cmd_initialize_emulated },
	{ "KILL0", 		CMD_TYPE_SPECIAL, cmd_kill },
	{ "KILL1", 		CMD_TYPE_SPECIAL, cmd_kill_emulated },
	{ "MOUNT", 		CMD_TYPE_DOS, cmd_mount_d64 },
	{ "MD64",		CMD_TYPE_DOS, cmd_maked64 },
	{ "MD",			CMD_TYPE_DOS, cmd_makedir },
	{ "RENAME",		CMD_TYPE_DOS, cmd_rename },
	{ "SCRATCH",	CMD_TYPE_DOS, cmd_scratch },
	{ "SETSAVE",	CMD_TYPE_SPECIAL, cmd_settingsave },
	{ "U0>",		CMD_TYPE_SPECIAL, cmd_changeaddr },
	{ "U1>",		CMD_TYPE_SPECIAL, cmd_changeaddr_emulated },

};

//void iec_open_channel();
void iec_open_file();
void iec_close_file();

const CHAR dir_end[]    = "BLOCKS FREE."; // The other zero is hidden as terminator to the string
const BYTE dir_end_len  = NR_OF_EL(dir_end) - 1;


static BYTE rx_len;
//static BYTE tx_len;
//static BOOL tx_end;

static BOOL atn;

static BYTE mem_io_len;

//static BOOL addressed;
static BYTE chan, cmd; // mode
static BYTE iec_addr;

#define IEC_NUM_FILES 2

static FIL	files[IEC_NUM_FILES];
static BOOL filesOpen[IEC_NUM_FILES];
static BYTE filesChan[IEC_NUM_FILES];  // which channel this file is linked to.. (to reopen if a file was not properly closed)

static CHAR current_dir[MAX_NAME_LEN + 1]; // One larger then the 16 the is req to fit terminator
static CHAR iec_inbuf[65]; // stores 65 because the last is always occupied by a zero
static BYTE track_counter;

struct t_iec_channel
{
    CHAR outbuf[256];
    CHAR *tx_buffer;  // points to the outbuf by default, but could point to a constant as well.
    BYTE mode;
    BYTE tr;
    BYTE tx_len;
    BYTE fileidx;     // points to a file pointer.. back linking
    FIL *file;        // points to a file pointer (of which there are only 2)  (NULL = closed)
    WORD rpos;        // current record # in rel file
    BYTE rec_len;     // record length for rel files
    BYTE offs;        // optional offset within record
    BOOL tx_end;
};

struct t_iec_channel iec_channels[16];
struct t_iec_channel *channel; // pointer to current channel

//static ENTRYADMIN entryadmin;
       DIRECTORY	iec_dir_cache;    // cannot be static, because we also need it in the stand alone loop!
static ENTRYNAME	iec_entry_name;

static BOOL auto_mount;
//static BYTE mount_request;
static BOOL suicide;
static BOOL suicide_emulated;
static BOOL reset;

static BOOL reset_emulated;

static BYTE cbm_medium_type;
DWORD cbm_medium_ptr;

#define CMD_LISTEN    0x20
#define CMD_TALK      0x40
#define CMD_UNLISTEN  0x3F
#define CMD_UNTALK    0x5F
#define CMD_OPENCHNL  0x60
#define CMD_OPEN      0xF0
#define CMD_CLOSE     0xE0

#define GPIOSETFALSE(a) GPIO_OUT=a
#define GPIOSETTRUE(a) GPIO_OUT=a

static BOOL send_dirline(BOOL);
static void next_data_out(void);
static void next_data_in(void);
static void enter_cbm(DIRENTRY *entry, BYTE mode);
static void set_rel_position(struct t_iec_channel *ch, WORD rpos, BYTE offset);

static BYTE iec_dir_change(DWORD cluster)
{
    static BYTE dot;

    dot = cfg_get_value(CFG_HIDE_DOT);
    return dir_change(cluster, &iec_dir_cache, dot);
}

static void cmd_automount (void)
{
	if(iec_entry_name.name_str[iec_entry_name.len - 1] == '0') {
		auto_mount = FALSE;
	}
	else {
		auto_mount = TRUE;
	}
}

static void cmd_memread (void)
{
	// TODO: implement memory if
	// pos 0 - 2 = command
	// pos 3     = High address
	// pos 4	 = Low address
	// pos 5     = Number of bytes
	mem_io_len = iec_inbuf[5] & 0xFF;
	if(mem_io_len == 0) mem_io_len++; 
	channel->mode = MODE_MEM_READ;
}

static void cmd_kill (void)
{
	config_in_ram = cfg_get_entry(CFG_IEC_ENABLE);
	config_in_ram->value = FALSE;

	last_error  = ERR_DOS;
	
	suicide = TRUE;
}

static void cmd_kill_emulated (void)
{
	config_in_ram = cfg_get_entry(CFG_1541_ENABLE);
	config_in_ram->value = FALSE;
	
	suicide_emulated = TRUE;
	reset_emulated = TRUE;
}



static void cmd_initialize (void)
{
	reset = TRUE;
}

static void cmd_initialize_emulated (void)
{
	config_in_ram = cfg_get_entry(CFG_1541_ENABLE);
	config_in_ram->value = TRUE;

	reset_emulated = TRUE;
}

static void cmd_changeaddr (void)
{
	BYTE addr;
	addr = iec_entry_name.name_str[3];
	if((addr >= 8) && (addr <= 30)) {

		iec_addr = addr;
		
		config_in_ram = cfg_get_entry(CFG_IEC_ADDR);
		config_in_ram->value = (WORD)addr;
		
		reset = TRUE;
		return;
	}
	
	last_error = ERR_SYNTAX_ERROR_CMD;
}

static void cmd_changeaddr_emulated (void)
{
	BYTE addr;
	addr = iec_entry_name.name_str[3];
	if((addr >= 8) && (addr <= 11)) {
		config_in_ram = cfg_get_entry(CFG_1541_ADDR);
		config_in_ram->value = (WORD)addr;

		addr -= 8;
	    GPIO_CLEAR  = DRIVE_ADDR_MASK;
    	GPIO_SET    = ((addr & 0x03) << DRIVE_ADDR_BIT);
		
		reset_emulated = TRUE;
		return;
	}
	
	last_error = ERR_SYNTAX_ERROR_CMD;	
}

static void cmd_settingsave (void)
{
	cfg_save_flash();
}


static void cmd_mount_d64 (void)
{
    static DIRENTRY *entry;
	static BYTE	types[] = {
							TYPE_D64,
							TYPE_GCR,
							TYPE_TERMINATOR // terminator
							};
	CHAR*	name = iec_entry_name.name_str;
	BYTE 	wp, tr, delay;

	if(iec_entry_name.len > MAX_NAME_LEN) {
		last_error = ERR_SYNTAX_ERROR_NAME; 
		return;
	}	
	
	entry = dir_find_entry(&iec_entry_name, types, &iec_dir_cache);
	if(entry == NULL) {
		last_error = ERR_FILE_NOT_FOUND;
		return;
	}
	
    sal_dir_index = ((WORD)entry - MAP1_ADDR) / sizeof(DIRENTRY);
//    printf("sal_dir_index = %d, because entry = %p.\n", sal_dir_index, entry);

    delay = (BYTE)cfg_get_value(CFG_SWAP_DELAY);

	f_close(&mountfile);
	iec_fres = f_open_direct(&mountfile, entry);
	if(iec_fres == FR_OK) {
        last_update(entry);
        if(entry->ftype == TYPE_D64) {
    		wp = (BYTE)(entry->fattrib & AM_RDO);
    		printf("wp = %d. (fa = %d)\n", wp, entry->fattrib);
            tr = load_d64(&mountfile, wp, delay);
        } else {
            if(!load_g64(&mountfile, 1, delay)) {
                last_error = ERR_FILE_TYPE_MISMATCH;
                return;
            }
        }
	}
	
	last_error = FF_TO_IEC[iec_fres];
	iec_dir_index = 0; 
//	mount_request = TRUE | wp << 1;
}

static void cmd_change_dir (void)
{
	CHAR* name = iec_entry_name.name_str;
    if(strncmp(name, "..", 2) == 0) { // one dir up
    	iec_dir_up();
	} 
	else if(strncmp(name, "/", 1) == 0) {
		iec_dir_depth = 1;
		iec_dir_up();
	}
	else {
       	iec_dir_down(NULL);
    }
}

static void cmd_scratch (void)
{
    static DIRENTRY *entry;
    DWORD dir_cluster;

	if(iec_dir_depth) {
		dir_cluster = iec_subdir_trace[iec_dir_depth-1];
	} 
	else {
	    dir_cluster = 0L;
	}

	track_counter = 0;
	if(iec_dir_cache.cbm_medium) {
		// NOT SUPPORTED yet
		last_error = ERR_WRITE_PROTECT_ON;
		return;
	}
	else {
		last_error = ERR_FILES_SCRATCHED;
		do {
			entry = dir_find_entry(&iec_entry_name, NULL, &iec_dir_cache);
			if(entry != NULL) {
				f_opendir_direct(&iec_dir, dir_cluster);
				iec_fres = f_unlink_direct(&iec_dir, entry);				
				if(iec_fres == FR_OK) {
					iec_dir_change(dir_cluster); // refresh
					track_counter++;
				}
			}
			else {
				break;
			}
		} while (1);
	}
}

static void cmd_rename (void)
{
	last_error = ERR_UNIMPLEMENTED;
}

static void cmd_copy (void)
{
	last_error = ERR_UNIMPLEMENTED;
}

static void cmd_makedir (void)
{
    DWORD dir_cluster;
	
	if(iec_dir_cache.cbm_medium) {
		last_error = ERR_DIRECTORY_ERROR;
		return;
	}

//    printf("Going to create dir with name: %s\n", iec_entry_name.name_str);
    if(iec_dir_depth) {
        dir_cluster = iec_subdir_trace[iec_dir_depth-1];
    }
	else {
        dir_cluster = 0L;
    }
	
    f_opendir_direct(&iec_dir, dir_cluster); 
	// DONT CALL ANY FUNCTION AFTER OPEN DIR DIRECT THAT TOUCHES THE IEC DIR OBJECT
	// OR ELSE THE CREATE DIRECTORY IS NOT ABLE TO CHECK IF THE FILE EXISTS
    iec_fres = f_create_dir_entry(&iec_dir, iec_entry_name.name_str, iec_entry_name.replace);	
	last_error = FF_TO_IEC[iec_fres];

	// refresh dir
	iec_dir_change(dir_cluster);
}

static void cmd_maked64 (void)
{
    DWORD dir_cluster;

	if(iec_dir_cache.cbm_medium) {
		last_error = ERR_DIRECTORY_ERROR;
		return;
	}

    printf("name = '%s', last 4: '%s'\n", iec_entry_name.name_str, &iec_entry_name.name_str[iec_entry_name.len-4]);
    // fix extension
    if((iec_entry_name.len < 4)||(strcmp(&iec_entry_name.name_str[iec_entry_name.len-4], ".d64")!=0)) {
        strcat(iec_entry_name.name_str, ".d64");
        iec_entry_name.len += 4;
    }
    printf("After fix: %s\n", iec_entry_name.name_str);
    
    if(iec_dir_depth) {
        dir_cluster = iec_subdir_trace[iec_dir_depth-1];
    }
	else {
        dir_cluster = 0L;
    }
	
    f_opendir_direct(&iec_dir, dir_cluster);
	// DONT CALL ANY FUNCTION AFTER OPEN DIR DIRECT THAT TOUCHES THE IEC DIR OBJECT
	// OR ELSE THE CREATE FILE ENTRY IS NOT ABLE TO CHECK IF THE FILE EXISTS
    iec_fres = f_create_file_entry(&iec_file, &iec_dir, iec_entry_name.name_str, D64_SIZE, iec_entry_name.replace, FALSE);
   	last_error = FF_TO_IEC[iec_fres];

	if(iec_fres == FR_OK) {
		iec_fres = d64_create(&iec_file, iec_entry_name.name_str);
        f_close(&iec_file);
	   	last_error = FF_TO_IEC[iec_fres];	
	}

	// refresh dir
	iec_dir_change(dir_cluster);
}
	
/*
    CHAR outbuf[256];
    CHAR *tx_buffer;  // points to the outbuf by default, but could point to a constant as well.
    BYTE mode;
    BYTE tr;
    BYTE tx_len;
    BYTE fileidx;     // points to a file pointer.. back linking
    FIL *file;        // points to a file pointer (of which there are only 2)  (NULL = closed)
    BOOL tx_end;
    BYTE rec_len;     // record length for rel files
*/
static
void cmd_position(void)
{
    WORD  rpos; //, bw; // wrlen
    BYTE ch, b;
    
    if(rx_len < 4) {
        last_error = ERR_SYNTAX_ERROR_GEN;
        // no effect on channel itself, command is wrong
        return;
    }

    ch = iec_inbuf[1]; // has already been checked for < 15!
//    file = iec_channels[ch].fileidx;
//    if(!filesOpen[file]) {

    if(!iec_channels[ch].rec_len) { // rel files have their record length set
        last_error = ERR_NO_CHANNEL; // odd, but this is what the 1541 does
        iec_channels[ch].mode = MODE_ERROR;
        return;
    }        
    if(!iec_channels[ch].file) {
        last_error = ERR_FILE_NOT_OPEN;
        iec_channels[ch].mode = MODE_ERROR;
        return;
    }

    rpos = (iec_inbuf[3] << 8)|iec_inbuf[2];

    b = 0;
    if(rx_len > 4) {
        b = iec_inbuf[4];
        if(b)
            b--;
    }
        
    if(rpos)
        set_rel_position(&iec_channels[ch], rpos-1, b);
    else
        set_rel_position(&iec_channels[ch], 0, b);
}

static
void set_rel_position(struct t_iec_channel *ch, WORD rpos, BYTE offs)
{
    DWORD fpos, to_add;
    WORD wrlen, bw;

    ch->rpos = rpos;
    ch->offs = offs;
    
    fpos = ch->rec_len * rpos;
    fpos += offs;
    fpos ++;  // compensate for record length byte at start of file
    
//    printf("Position REL file to record #%d (filepos = %ld).\n", rpos, fpos);

    if(ch->file->fsize <= fpos) { // format
        to_add = fpos - ch->file->fsize;
        to_add += ch->rec_len; // create one full entry at the end
//        printf("Adding %ld bytes to the rel file.\n", to_add);
        f_lseek(ch->file, ch->file->fsize);
        memset(ch->outbuf, 0x00, 256);
        ch->outbuf[0] = 0xFF;
        while(to_add) {
            wrlen = (to_add>ch->rec_len)?ch->rec_len:(WORD)to_add;
            f_write(ch->file, ch->outbuf, wrlen, &bw);
            to_add -= wrlen;
        }
    }
    f_lseek(ch->file, fpos);
}



static
void enter_cbm(DIRENTRY *entry, BYTE mode)
{
    if(iec_dir_depth+1 < MAX_DEPTH) {
        iec_subdir_trace[iec_dir_depth++] = entry->fcluster;
        f_open_direct(&iec_file, entry);
		if(mode == TYPE_T64) {
			iec_fres = t64_dir(&iec_file, &iec_dir_cache, TRUE);
		}
		else {
			iec_fres = d64_dir(&iec_file, &iec_dir_cache, FALSE, mode); // not just ascii, but all entries
		}
        if(iec_fres != FR_OK)  {
			last_error = FF_TO_IEC[iec_fres];
			iec_dir_up();
        }
    }
}


/*
-------------------------------------------------------------------------------
							iec_dir_up
							==========
  Abstract:

	Moves the emulated IEC a directory up

-------------------------------------------------------------------------------
*/
void iec_dir_up(void)
{
    if(iec_dir_depth == 0)
        return;
    iec_dir_depth--;
	if(iec_dir_cache.cbm_medium) {
		f_close(&iec_file);	// ale a cbm medium implies the iec_file pointer is used
	}
    if(iec_dir_depth == 0) {
        iec_fres = iec_dir_change(0L);
		data_strnicpy(current_dir, (CHAR *)&msg73[(NR_OF_EL(msg73) - 1) - NR_OF_EL(current_dir) - 1], NR_OF_EL(current_dir)); // todo find the volume name
	}
    else {
        iec_fres = iec_dir_change(iec_subdir_trace[iec_dir_depth - 1]);
		data_strnicpy(current_dir, (CHAR *)&msg73[(NR_OF_EL(msg73) - 1) - NR_OF_EL(current_dir) - 1], NR_OF_EL(current_dir)); // todo find the current dir name
    }
	
	last_error = FF_TO_IEC[iec_fres];	
}

/*
-------------------------------------------------------------------------------
							iec_dir_down
							============
  Abstract:

	Moves the emulated IEC a directory down

-------------------------------------------------------------------------------
*/
BOOL iec_dir_down(DIRENTRY *suggest)
{
    static DIRENTRY *entry;
	
	static BYTE	types[] = {
							TYPE_DIR,
							TYPE_D64,
							TYPE_T64,
							TYPE_TERMINATOR // terminator
						};
	
	CHAR*	name = iec_entry_name.name_str;

    if(suggest)
        entry = suggest;
    else    
	    entry = dir_find_entry(&iec_entry_name, types, &iec_dir_cache);
	    
	if(entry == NULL) {
		last_error = ERR_FILE_NOT_FOUND;
		return FALSE;
	}

	data_strnicpy(current_dir, entry->fname, NR_OF_EL(current_dir) - 1);
	current_dir[16] = 0;
	if(data_strnlen(entry->fname , MAX_ENTRY_NAME) > 16)
		current_dir[15] = '*';
	
    switch(entry->ftype) {
	    case TYPE_DIR:

			if(iec_dir_depth+1 < MAX_DEPTH) {
				iec_subdir_trace[iec_dir_depth++] = entry->fcluster;
				iec_fres = iec_dir_change(entry->fcluster);
				if(iec_fres != FR_OK) { 		  
					last_error = FF_TO_IEC[iec_fres];
					iec_dir_up();
					return FALSE;
				}
                sal_dir_index = 0;
			}
	        break;
	    case TYPE_D64:
		case TYPE_D71:
	    case TYPE_D81:			
	    case TYPE_T64:			
            sal_dir_index = 1;
            enter_cbm(entry, entry->ftype);
			break;
	    default:			
	        printf("Can't enter this type.\n");
			last_error = ERR_FILE_TYPE_MISMATCH;
			return FALSE;
    }
	last_error = FF_TO_IEC[iec_fres];
	return TRUE;
}


/*
-------------------------------------------------------------------------------
							open_for_read
							=============
  Abstract:

	opens the file and starts transferring (this might not be a good idea ;-)
	
-------------------------------------------------------------------------------
*/
static
void open_for_read(BYTE type)
{
	ENTRYNAME temp_entry_name;
    static DIRENTRY *entry;
	static BYTE	types[] = {
							TYPE_PRGD64,
							TYPE_PRGD71,								
							TYPE_PRGD81,
							TYPE_PRGT64,
							TYPE_PRG,
							TYPE_UNKNOWN,
							TYPE_TERMINATOR
							};

	iec_fres = FR_OK;

	if(!iec_dir_cache.cbm_medium) {
		petscii2ascii (iec_entry_name.name_str, iec_entry_name.name_str); // was 2x inbuf
	}	

	//dir_analyze_name(iec_inbuf, MAX_NAME_LEN, &iec_entry_name); // To handle wildcards

    //dump_entryname(&iec_entry_name);

	if(iec_entry_name.illegal) {
		last_error = ERR_SYNTAX_ERROR_NAME;
        channel->mode = MODE_ERROR;
		return;
	}
	if(iec_entry_name.unit != 0) {
		last_error = ERR_DRIVE_NOT_READY; // Not having another unit :)
        channel->mode = MODE_ERROR;
		return;
	}

	if(iec_entry_name.command_len == 1) {
		if(iec_entry_name.command_str[0] == '$') {		
			channel->mode = MODE_DIR_OPEN;
			return;
		}
	}
	else {
		if(iec_entry_name.command_len != 0) {
			last_error = ERR_SYNTAX_ERROR_NAME;
            channel->mode = MODE_ERROR;
			return;
		}	
		
		if((iec_entry_name.len == 0) && (!iec_entry_name.wildcard)) {
            channel->mode = MODE_ERROR;
			last_error = ERR_SYNTAX_ERROR_NONAME;
			return;
		}
		if(iec_entry_name.len > 0) {
			if(iec_entry_name.name_str[0] == '$') {
				dir_analyze_name(&iec_entry_name.name_str[1], MAX_NAME_LEN, &temp_entry_name); // To handle wildcards
				// Ignoring errors for now as the above already filtered the lot
				if(temp_entry_name.len > 0) { // interpret this as pattern matching
					temp_entry_name.pattern = TRUE;
				}
				memcpy(&iec_entry_name, &temp_entry_name, sizeof(ENTRYNAME));
				channel->mode = MODE_DIR_OPEN;
				return;
			}
		}
		entry = dir_find_entry(&iec_entry_name, NULL, &iec_dir_cache); // types
		if(entry == NULL) {
            printf("%s not found, trying ", iec_entry_name.name_str);
            strcat(iec_entry_name.name_str, ext_per_type[type]);
            printf("%s.\n", iec_entry_name.name_str);
    		iec_entry_name.len += 4; // length of extension added (including .)
    		entry = dir_find_entry(&iec_entry_name, NULL, &iec_dir_cache); // types
    		if(entry == NULL) {
    			last_error = ERR_FILE_NOT_FOUND;
    			channel->mode = MODE_ERROR;
    			return;
            }
		} else if(entry->ftype == TYPE_DIR) {
		    last_error = ERR_DIRECTORY_ERROR;
			channel->mode = MODE_ERROR;
			return;
        }		    
	}
		
//    printf("Entry type = %d.\n", entry->ftype);
    
    switch(entry->ftype) {
	    case TYPE_PRGD64:
			if(auto_mount) {
				// this is bad! we need to mount the file we are actually in!!!
				// Step 1. create a bogus mount command
				sprintf(iec_inbuf,"MOUNT:%s", current_dir);
				// Step 2. up one directory so we can find the file (TODO: implement path tracing)
				iec_dir_up();
				// Step 3. execute the mount command
				handle_dos_cmd(); // The name after "MOUNT:" will end up in iec_entry_name.name_str
				// Step 4. go onto the D64 again
				sprintf(iec_inbuf,"CD:%s", iec_entry_name.name_str);	// create bogus command
				handle_dos_cmd(); // dir down again
				// Step 5. pray the entry pointer is correct again :S
			}
			// FALLTROUGH
		case TYPE_PRGD71:
		case TYPE_PRGD81:			
			cbm_medium_type = entry->ftype;
	        cbm_medium_ptr = entry->fcluster;
			channel->mode = MODE_D64PRG_READ;
			break;
	    case TYPE_PRGT64:
	        cbm_medium_ptr = (DWORD)entry; // eui being a bad programmer here. now look away please.
			channel->mode = MODE_T64PRG_ADDR;
			break;
	    case TYPE_PRG:
	    default:
            printf("open file.\n");
			iec_fres = f_open_direct(channel->file, entry);
            channel->file->flag &= ~FA_WRITE; // do not allow writes
			if(iec_fres == FR_OK) {
				filesOpen[channel->fileidx] = TRUE;
				channel->mode = MODE_PRG_READ;
				break;
			}

			channel->mode = MODE_ERROR;
			
#if 1//IEC_DEBUG
			printf("Error opening %s\n\r", iec_entry_name.name_str);
#endif			
			break;
    }
	last_error = FF_TO_IEC[iec_fres];
}


/*
-------------------------------------------------------------------------------
							open_for_write
							==============
  Abstract:

	creates the file and sets up data reception
	
-------------------------------------------------------------------------------
*/
static
void open_for_write(BYTE type, BYTE access)
{
//    static DIRENTRY *entry;
    DWORD dir_cluster;
	CHAR* name = iec_inbuf;
    BOOL  append;
    WORD bt;
    BYTE rl;
    
	if(iec_dir_cache.cbm_medium) {
		last_error = ERR_WRITE_ERROR;
		channel->mode = MODE_ERROR;
		return;
	}
	else {
		petscii2ascii (iec_inbuf, iec_inbuf);
	}
	
//	dir_analyze_name(name, MAX_NAME_LEN, &iec_entry_name); // For pattern matching and stuff (already done in iec_open_file)
	if( iec_entry_name.pattern ||
		iec_entry_name.command_len != 0 || // THIS IS ALLOWED BUT NOT YET IMPLEMENTED
		iec_entry_name.wildcard || 
		iec_entry_name.illegal ||
		(iec_inbuf[0] == '$')) {

		last_error = ERR_SYNTAX_ERROR_NAME;
		channel->mode = MODE_ERROR;
		return;
	}
	
	if(iec_dir_depth) {
    	dir_cluster = iec_subdir_trace[iec_dir_depth-1];
    } 
	else {
        dir_cluster = 0L;
	}

    // if file is of type REL, then see if it exists (this is a read/write access then)
    printf("Filename = %s\n", iec_entry_name.name_str);
    append = (type == 3)||(access & ACC_APPEND);
    
/*
    if() {
    	entry = dir_find_entry(&iec_entry_name, NULL, &iec_dir_cache); // types
    	if(entry) {
            printf("Append found existing entry...\n");
            iec_fres = f_open_direct(channel->file, entry);
        	last_error = FF_TO_IEC[iec_fres];
            if(iec_fres == FR_OK) {
            	filesOpen[channel->fileidx] = TRUE;
                channel->file->flag |= FA_WRITE;
                printf("Existing file is now open! Write flag set.\n");
                if(access & ACC_APPEND) {
                    iec_fres = f_lseek(channel->file, channel->file->fsize);
                    printf("Append result: %d. FilePos = %ld\n", iec_fres, channel->file->fsize);
                }
                return;
            }
            channel->mode = MODE_ERROR;
            return; // file is now open, it already existed, so we don't need to do anything else
        } else {
            printf("Append failed to open existing file.\n");
        }
    }
*/    
    // we have to actually create the file..
    f_opendir_direct(&iec_dir, dir_cluster); // this is not the same as change dir; this is just to fill in the iec_dir structure.
                                             // the directory cache is still ok.
	iec_fres = f_create_file_entry(channel->file, &iec_dir, iec_entry_name.name_str, 0L, iec_entry_name.replace, append);
	if(iec_fres != FR_OK) {
printf("File error\n");
		last_error = FF_TO_IEC[iec_fres];
		channel->mode = MODE_ERROR;
    	filesOpen[channel->fileidx] = TRUE;  // hack
        iec_close_file();
		return;
	}

	filesOpen[channel->fileidx] = TRUE;

    if(type==3) {
        if(!channel->file->fsize) {
            printf("rel file length is zero... did I just create it?\n");
            f_write(channel->file, &channel->rec_len, 1, &bt);
        } else { // reopen, check size!
            rl = 0;
            f_lseek(channel->file, 0L); // start of file
            f_read(channel->file, &rl, 1, &bt);  // should not fail, because we know the length of file is > 0
            if(rl != channel->rec_len) {
                last_error = ERR_RECORD_NOT_PRESENT;
                printf("bad record size\n");
                iec_close_file(); // no further access possible
                return;
            } 
        }
        set_rel_position(channel, 0, 0);
        channel->mode = MODE_REL_READ;
    }
		
	// refresh dir
	iec_dir_change(dir_cluster);

	last_error = FF_TO_IEC[iec_fres];

}

/*
-------------------------------------------------------------------------------
							handle_dos_cmd
							==============
  Abstract:

	Commands to the drive at chnl 15 are specials and are handled here
	
-------------------------------------------------------------------------------
*/
void handle_dos_cmd()
{
	BOOL special_only;
	CHAR* command_str;
	BYTE command, command_len, type;

	last_error = ERR_OK; // Others will set this to something in case of trouble

	// -----------------------------------------------------------------------------/
	// FAST COMMANDS (OR THE AR DOES NASTY THINGS)									/
	// HANDLE THESE COMMANDS SWIFTLY OR THE AR WILL ASSUME YOU ARE A 1541			///
	// IT WILL START UPLOADING STUFF AND HANG!!!!!!									///
	if(strncmp(iec_inbuf, "M-R", 3) == 0) {											///
		cmd_memread();																///
		return;																		///
	}																				///
	//  ---	END OF FAST COMMANDS ----------------------------------------------------//
		///////////////////////////////////////////////////////////////////////////////

    // OTHER binary commands, such as the positioning of REL files
    if((iec_inbuf[0] == 'P')&&(iec_inbuf[1] < 15)) {
        cmd_position();
        return;
    }
		
	// OUR IEC/SD Command interpreter
	dir_analyze_name(iec_inbuf, NR_OF_EL(iec_inbuf), &iec_entry_name);

    //dump_entryname(&iec_entry_name);

	if(iec_entry_name.unit != 0) {
		last_error = ERR_DRIVE_NOT_READY; // Not having another unit :)
		return;
	}
	
	special_only = iec_entry_name.command_str == NULL;
	if(!special_only) {
		if(iec_entry_name.illegal) {
			last_error = ERR_SYNTAX_ERROR_GEN;
			return;
		}
	}
	
	if(iec_entry_name.len == 0) {
		if(!iec_entry_name.wildcard) {
			last_error = ERR_SYNTAX_ERROR_NONAME;
			return;
		}
	}
	else {
		if(!iec_dir_cache.cbm_medium && !special_only) {
			petscii2ascii (iec_entry_name.name_str, iec_entry_name.name_str);
		}
	}
	for(command = 0; command < NR_OF_EL(commands); command++) {
		command_str = commands[command].cmdStr;
		command_len = strlen(command_str);
		type = commands[command].type;
		if(type == CMD_TYPE_DOS) {
			if(special_only) {
				continue;
			}
			if(strncmp(iec_entry_name.command_str, command_str, iec_entry_name.command_len) == 0) {
				break; // This is the command (regardless of matching duplicates)
			}
		}
		else {
			if(iec_entry_name.len) {
				if(strncmp(iec_entry_name.name_str, command_str, iec_entry_name.len) == 0) {
					break; // This is the command (regardless of matching duplicates)
				}
			}
		}
	}

	if(command <  NR_OF_EL(commands)) {
		printf("found %s",command_str);
		(*commands[command].proc)();
	}
	else {
		last_error = ERR_SYNTAX_ERROR_CMD;
	}
}

/*
-------------------------------------------------------------------------------
							handle_msg
							==========
  Abstract:
	Handles a message that comes in over the IEC channel
	
-------------------------------------------------------------------------------
*/
static
void handle_msg(void)
{
//    printf("('%s'.c%d.e%d)", iec_inbuf, chan, eoi);

	switch(cmd) {
/* The way cmd_open and cmd_openchnl are used in practice is unclear.
   eg. fc3 uses open_chnl to transfer the command while ar uses cmd_open */

		case CMD_OPEN:
            if(chan == 15) { // command channel
                if(!eoi) {
                    last_error = ERR_SYNTAX_ERROR_CMDLENGTH;
                    channel->mode = MODE_ERROR;
                    return;
                }                    
    			handle_dos_cmd();
            } else {
                iec_open_file();
            }
            break;
            			
		case CMD_OPENCHNL:
            if(chan == 15) { // command channel
                if(!eoi) {
                    last_error = ERR_SYNTAX_ERROR_CMDLENGTH;
                    channel->mode = MODE_ERROR;
                    return;
                }                    
    			handle_dos_cmd();
            } else {
				next_data_in();
			}
			break;
			
		case CMD_CLOSE:
            UART_DATA = '$';
            iec_close_file();
/*
			if(filesOpen[chan]) {
				f_close(&files[chan]);
				filesOpen[chan] = FALSE;
			}
*/
			break;
			
		default:
			// WTF?!?!
			break;
	}
}


/*
-------------------------------------------------------------------------------
							get_last_error
							==============
  Abstract:
	 
	Sets global pointers to a rellevant status message

-------------------------------------------------------------------------------
*/
static
void get_last_error(void)
{
	BYTE i;

	for(i = 0; i < NR_OF_EL(last_error_msgs); i++) {
		if(last_error == last_error_msgs[i].nr) {
			channel->tx_len = sprintf(channel->outbuf,"%02d,%s,%02d,00\015", last_error,last_error_msgs[i].msg, track_counter);
			track_counter = 0;
		}
	}
    channel->tr         = 0;
	channel->tx_buffer 	= channel->outbuf;
    channel->tx_end		= TRUE;
	last_error 	= ERR_OK;
}

/*
-------------------------------------------------------------------------------
							next_data_out
							=============
  Abstract:

	Sends the data via IEC

-------------------------------------------------------------------------------
*/
static
void next_data_out(void)
{
	WORD t;
	WORD rl;
	
    UART_DATA = '*';
    
    channel->tx_len = 0;

    if(chan == 0x0F) { // TODO REVISE THIS

		switch(channel->mode) {
			case MODE_MEM_READ:
				// TODO: implement mem i/f

				//* MAKE THIS A FAST FUNCTION BECAUSE TOO MUCH HASSLE HERE CAUSES MALFUNCTION
				// THE ACTION REPLAY WILL ASSUME YOU ARE A 1541 IF YOU DON'T ANSWER SWIFTLY

				
				memset(channel->outbuf, 0x00, mem_io_len);

				channel->tx_buffer = channel->outbuf;
				channel->tx_end	   = TRUE;
				channel->tx_len	   = mem_io_len;
				channel->mode      = MODE_IDLE;
				break;

			default: 
                UART_DATA = '=';
			    get_last_error();
/*				if((mode == MODE_DIR_READ) || (mode == MODE_DIR_OPEN)) {
					mode = MODE_IDLE;
				}
*/				break;
		}
		return;
    }
	else {
		switch(channel->mode) {
			case MODE_DIR_OPEN:
				send_dirline(TRUE);
				channel->mode = MODE_DIR_READ;
				break;				
			case MODE_DIR_READ:
		        if(!send_dirline(FALSE)) {
	 		        channel->mode = MODE_IDLE;
	         	}
				break;
				
			case MODE_REL_READ:
				UART_DATA = 'r';
                rl = (WORD)channel->rec_len;
                if(channel->offs <= rl)
                    rl -= channel->offs;
				iec_fres = f_read(channel->file, (void*)channel->outbuf, channel->rec_len, &t);
				channel->outbuf[channel->rec_len] = '\0';
				channel->tx_len = (BYTE)strlen(channel->outbuf);
				channel->tx_end = TRUE;
				channel->tx_buffer = channel->outbuf;
                set_rel_position(channel, ++(channel->rpos), 0);
                break;

			case MODE_PRG_READ:
                UART_DATA = 'p';
                
				iec_fres = f_read(channel->file, (void*)channel->outbuf, NR_OF_EL(channel->outbuf)-1, &t);
				channel->tx_len = (BYTE)t;
				channel->tx_buffer = channel->outbuf;
				channel->tx_end = (channel->tx_len != NR_OF_EL(channel->outbuf)-1) || (iec_fres != FR_OK);
				if(channel->tx_end) {
					channel->mode = MODE_IDLE;
				}
				break;

			case MODE_D64PRG_READ:
				iec_fres = d64_readblock(	&iec_file,
											(BYTE*)channel->outbuf,
											&channel->tx_len,
											cbm_medium_ptr,
											&cbm_medium_ptr,
											cbm_medium_type); // Actually this may not even be a D64 :-)
				channel->tx_buffer = channel->outbuf;
				last_error = FF_TO_IEC[iec_fres];
				if(iec_fres == FR_OK) {
					channel->tx_end = (channel->tx_len != 254);
					if(channel->tx_end) {
						channel->mode = MODE_IDLE;
					}
					break;
				}	
				else {
					channel->mode = MODE_ERROR;
				}
				break;

			case MODE_T64PRG_ADDR:
				{
				    DIRENTRY *entry = (DIRENTRY*)cbm_medium_ptr;
    				channel->outbuf[0] = entry->fname[20];
    				channel->outbuf[1] = entry->fname[21];
    				channel->tx_len = 2;
    				channel->tx_end = FALSE;
    				cbm_medium_ptr = entry->fcluster;
    				channel->mode = MODE_T64PRG_READ;				
                }
				break;

			case MODE_T64PRG_READ:
				iec_fres = t64_readblock(&iec_file,
				                        (BYTE*)channel->outbuf,
				                        &channel->tx_len,
				                        cbm_medium_ptr,
				                        &cbm_medium_ptr);
				last_error = FF_TO_IEC[iec_fres];
				if(iec_fres == FR_OK) {
					channel->tx_end = (channel->tx_len != 255);
					if(channel->tx_end) {
						channel->mode = MODE_IDLE;
					}
					break;
				}	
				else {
					channel->mode = MODE_ERROR;
				}
				break;
				
			case MODE_ERROR:
            case MODE_IDLE:
				channel->tx_len = 0;
                channel->tr = 0;
				shutup = TRUE; // THIS WILL SHUT HER UP!
				channel->mode = MODE_IDLE;
				break;

	 		default:
				printf("Unknown mode!\n");
                shutup = TRUE;
				channel->tx_len = 0;
                channel->tr = 0;
				break;
	    }
	}
}   

/*
-------------------------------------------------------------------------------
							next_data_in
							============
  Abstract:

	Stores data from the IEC to file

-------------------------------------------------------------------------------
*/
static
void next_data_in(void)
{
	WORD bytes_written;
//	DWORD dir_cluster;
	
	if(filesOpen[channel->fileidx]) {
        if(channel->rec_len) { // is rel file
            if(rx_len > channel->rec_len) {
                last_error = ERR_OVERFLOW_IN_RECORD;
            }
            iec_inbuf[rx_len++] = '\0'; // glue zero 0 at the end
            if(rx_len > channel->rec_len) {
                f_write(channel->file, iec_inbuf, (WORD)channel->rec_len, &bytes_written);
            } else {
                f_write(channel->file, iec_inbuf, rx_len, &bytes_written);
            }
            // go to the next record
            set_rel_position(channel, ++(channel->rpos), 0);
        } else {
    		iec_fres = f_write(channel->file, iec_inbuf, (WORD)rx_len, &bytes_written);
        }
//		printf("*W%d,%d,%dW*", chan, rx_len, bytes_written);
		if((iec_fres != FR_OK) || (rx_len != bytes_written)) { //   || eoi) {  not at EOI!!
//			if(iec_fres == FR_OK) {
//				iec_fres = f_write(&files[chan], &iec_inbuf[rx_len], 1, &bytes_written); // append zero
//			}
			last_error = FF_TO_IEC[iec_fres]; // Convert the error 
//            iec_close_file();
		}
	} else {
	    printf("Error: trying to write data to file that is not open.\n");
	}
}

/*
-------------------------------------------------------------------------------
							iec_init
							========
  Abstract:

	(re-)initialize IEC

  Parameters
	addr:		IEC address
	en:         Enable
	
-------------------------------------------------------------------------------
*/
void iec_init(BYTE addr, BOOL en)
{
	static BOOL reinit = FALSE;
	BYTE i;

    printf("iec init called. ID=%d EN=%c\n", addr, en?'Y':'N');
	if(reinit) {
		// always do this before resetting cbm_medium or your file is not closed!!!!!
		for(i = 0; i < NR_OF_EL(filesOpen); i++) {
			if(filesOpen[i]) {
				f_close(&files[i]);
				filesOpen[i] = FALSE;
			}
		}
        for(i = 0; i < 16; i++) {
            iec_channels[i].tx_len = 0;
            iec_channels[i].tr     = 0;
            iec_channels[i].mode   = MODE_IDLE;
            iec_channels[i].file   = NULL;
            iec_channels[i].tx_buffer = iec_channels[i].outbuf;
        }

		if(iec_dir_cache.cbm_medium) {
			f_close(&iec_file);
			iec_dir_cache.cbm_medium = FALSE;
		}
	}
    
	rx_len		= 0;
    listen      = FALSE;
    talk        = FALSE;
    talking     = FALSE;
	shutup		= FALSE;
	eoi			= FALSE;
	chan		= 15;
    channel     = &iec_channels[15];
	cmd			= 0;
	atn 		= FALSE;
    iec_addr    = addr;
	auto_mount	= FALSE;
//	mount_request = FALSE;
	iec_dir_cache.dir_address = IEC_DIR_ADDR;
	
	iec_dir_depth = 1;
	iec_dir_up();
	if(!reinit) {
		last_error 	= ERR_DOS;
	}
	else {
		last_error 	= ERR_OK;
	}
		
    // enable state machine
    IEC_ADDR = addr;
    if(en) {
        IEC_CTRL    = IEC_CTRL_READY;
    	suicide		= FALSE;
	    reinit 		= TRUE;

    } else {
        IEC_CTRL    = IEC_CTRL_KILL | IEC_CTRL_SWRESET;
    	suicide		= TRUE;
	    reinit 		= FALSE;
		last_error  = ERR_DOS;
    }
}


/*
-------------------------------------------------------------------------------
							iec_poll
							========
  Abstract:

	Poll for incoming data over the IEC

  Parameters
	addr:		IEC address
	
-------------------------------------------------------------------------------
*/
void iec_poll()
{
	static BYTE status;
	static BYTE data;
//    static BYTE tr;
	static BOOL tx_done;
	
	status = IEC_STAT;
	data = IEC_DATA;

	// First see what the error indicator must do. This depends on the yield of the last poll.
	if((last_error != ERR_OK) && (last_error != ERR_DOS)) {
		GPIO_SET = IEC_IF_ERROR; // Bad! set error.
	}
	else {
		GPIO_CLEAR = IEC_IF_ERROR; // No problems. Clear error if any
	}

    talk   = status & IEC_CTRL_TALKER;
    listen = status & IEC_CTRL_LISTENER;
    atn    = FALSE;
    
	if(status & IEC_STAT_DATA_AV) {
		atn = status & IEC_STAT_ATN;
    	if(atn) {
			cmd = data & 0xF0;
//			addressed = ((data & 0x1F) == iec_addr);
            UART_DATA = '!';
//            uart_hex(data);
			switch(cmd) {
/*
				case 0x20:
				case 0x30:
                    printf("Listen bytes shouldn't be handled here!\n");
//					listen = addressed;
//                    UART_DATA = listen?'L':'l';
//					talk = FALSE;
					break;
				case 0x40:
				case 0x50:
                    printf("Talk bytes shouldn't be handled here!\n");
//					talk = addressed;
//                    UART_DATA = talk?'T':'t';
//					listen = FALSE;
					break;
*/
                case 0x60: // open channel
                    UART_DATA = '[';
                    chan = data & 0x0F;
                    channel = &iec_channels[chan];
                    if(chan == 15) {
                        channel->tx_len = 0; // stop transmitting what we were transmitting
                    }
                    break;
                case 0xE0: // close file
                    chan = data & 0x0F;
                    channel = &iec_channels[chan]; // should we do this?
                    UART_DATA = '}';
                    iec_close_file();
                    break;
                case 0xF0: // open file
                    chan = data & 0x0F;
                    channel = &iec_channels[chan];
                    UART_DATA = '{';
                    break;
				default:
                    UART_DATA = '?';
					break;
			}
    	}
		else { // if (listen) { -- we should be listener, otherwise we don't get the bytes here!
            if(!listen) {
                printf("Hey, we got a byte, but we are not listener?\n");
            }
#if 0
            uart_hex(rx_len);
            uart_hex(data);
            UART_DATA = ',';
#endif
			iec_inbuf[rx_len] = data;
			rx_len++;
			eoi = (status & IEC_STAT_EOI);
	    	if((rx_len == (NR_OF_EL(iec_inbuf) - 1)) || eoi) { // If the buffer is full or if the last byte was received.
		    	iec_inbuf[rx_len] = 0; // so you can do string compares if you want
//                printf("\nmsg: '%s'\n", iec_inbuf);
	    	    handle_msg();
				rx_len = 0;
		    }
		}
		
    	IEC_CTRL = 	//(listen ? IEC_CTRL_LISTENER : 0) |
    				//(talk ? IEC_CTRL_TALKER : 0) | 
    				(suicide ? IEC_CTRL_KILL : 0) |
    				IEC_CTRL_READY;
		
//		return;

	}
	else if(talk) {
	    if(status & IEC_STAT_CLR_TO_SEND) { // Is previous data sent?
            if(status & IEC_STAT_BYTE_SENT) {
                channel->tr++;
            }
            
    		if(channel->tr >= channel->tx_len) { // is there nothing left in buffer to say
              	channel->tr = 0;	// reset counter					
    	        next_data_out(); // see if there is something to say				
    			tx_done = (channel->tx_len == 0);
            }
    
    		if(channel->tr != channel->tx_len) {
    			tx_done = (channel->tr == channel->tx_len - 1) && (channel->tx_end);
    			
                data = channel->tx_buffer[channel->tr];
#if 0
                if(data >= 0x20) {
                    UART_DATA = data;
                } else {
                    UART_DATA = '.';
                }
#endif
                IEC_DATA = data;
//    			channel->tr++;    
    		}
    		

/*
            TIMER = 20;
            while(TIMER)
                ;
*/
//            UART_DATA = (shutup)?'.':'>';
        	IEC_CTRL = 	\
        			IEC_CTRL_DATA_AV |
        			(tx_done ? IEC_CTRL_EOI : 0) |
        			(shutup ? IEC_CTRL_NODATA : 0) |
        			IEC_CTRL_READY;
        				
        	// Reset the 'throw away after use variable'
        	shutup = \
        	tx_done = FALSE;	
        }
	}

/*
	if(mount_request && !listen) {
		// BAD HACK!!!!!BAD BAD See ISSUE_LIST.txt
		GPIOSETFALSE(WRITE_PROT);// Darken as the floppy is extracted
		DELAY5US(200); 	 	 //  for 1000ms
		GPIOSETTRUE(WRITE_PROT); // Lighten as the floppy is gone
		DELAY5US(200); 	 	 //  for 1000 ms
		GPIOSETFALSE(WRITE_PROT);// Darken as the floppy is inserted
		if(!((mount_request >> 1) & AM_RDO)) { // Readonly?
			DELAY5US(200); 	 	 //  for 1000 ms
			GPIOSETFALSE(WRITE_PROT);// lighten as the floppy is not protected
		}
		mount_request = FALSE;
	} 
	else */
	if(reset) {
		// reinit
		iec_init(iec_addr, TRUE); // REVERT TO START
		reset = FALSE;
	}
	else if(reset_emulated) {
		// RESET the emulated 1541
		GPIO_SET2 = DRIVE_RESET;
		if(!suicide_emulated) {
			DELAY5US(10);
			GPIO_CLEAR2 = DRIVE_RESET;
		}
		suicide_emulated = FALSE; // Init will revive her
		reset_emulated = FALSE;
	}
}

/*
-------------------------------------------------------------------------------
							send_dirline
							============
  Abstract:

	Sends the directory per line as expected from an IEC device
	It can be listed in basic.
	
  Parameters
	first:		Indicated if the first line is to be send
	

  Return:
	TRUE:		When there are more lines
	FALSE:		When it is the last line
-------------------------------------------------------------------------------
*/
static
BOOL send_dirline(BOOL first)
{
    static WORD blocks;      
    static DIRENTRY *entry;    

	BYTE spaces, b;
	BOOL match;
	
	if(first) {
		if(!card_det) {
			printf("no disc\n");
			data_strncpy(current_dir, "no disc", 16);
			iec_dir_cache.cbm_medium = FALSE;
			last_error = ERR_DRIVE_NOT_READY;
		}
		else {
			last_error = ERR_OK;
		}
		// setup
        blocks = 0;
		iec_dir_index = 0;
		*((WORD*)&channel->outbuf[0]) = 0x0401; // Start address
		*((WORD*)&channel->outbuf[2]) = 0x0101; // Daft filler
		channel->outbuf[4] = 0;
		channel->outbuf[5] = 0;
        channel->outbuf[6] = 18;  // reverse code
        channel->outbuf[7] = 34;  // "
        memset(&channel->outbuf[8], 32, 27);
		if(iec_dir_cache.cbm_medium) {
		    entry = dir_getentry(iec_dir_index, &iec_dir_cache);   
			iec_dir_index ++;
			data_strncpy(&channel->outbuf[8], entry->fname, 16); //n->ni
		}
		else {
	        ascii2petscii(&channel->outbuf[8],current_dir); // current dir is always in ascii
		}
		b = data_strnlen(&channel->outbuf[8], 16);
		spaces =  16 - b;
		memset(&channel->outbuf[8 + b],' ', spaces);
        channel->outbuf[24] = 34;
        channel->outbuf[25] = ' ';
        channel->outbuf[26] = '0';
        channel->outbuf[27] = '1';        
        channel->outbuf[28] = ' ';
        channel->outbuf[29] = '2';
        channel->outbuf[30] = 'A';  
        channel->outbuf[31] = 0;      
        channel->tx_buffer = channel->outbuf;
        channel->tx_len    = 32;
        channel->tx_end    = FALSE;
        return TRUE;
    } 
	else {
		match = FALSE;		
		while(!match) {
			if((last_error == ERR_DRIVE_NOT_READY) || (!card_det)) {
				entry = 0;
			}
			else {
			    entry = dir_getentry(iec_dir_index, &iec_dir_cache);   
				iec_dir_index ++;
			}
			if((entry) && (entry->fname[0]) && (iec_dir_index)) { // there is a name
				if(iec_entry_name.pattern | iec_entry_name.wildcard) {
					match = dir_match_name(entry, &iec_entry_name, TRUE);
				}
				else {
					match = TRUE;
				}
				if(match) {	
				    blocks = (WORD)((entry->fsize + 253) / 254);
		            if(blocks > 9999)  blocks = 9999;
		            *((WORD*)&channel->outbuf[4]) = blocks;
		            spaces = 0;
		            if(blocks < 1000)  spaces ++;
		            if(blocks < 100)   spaces ++;
		            if(blocks < 10)    spaces ++;
		            memset(&channel->outbuf[6], 32, 27); // clear line
		            channel->outbuf[6 + spaces] = 34;
					if(!iec_dir_cache.cbm_medium) {
    					data_strnicpy(&channel->outbuf[7 + spaces], entry->fname, MAX_ENTRY_NAME);
                    } else {
    					data_strncpy(&channel->outbuf[7 + spaces], entry->fname, MAX_ENTRY_NAME);
                    }
					channel->outbuf[7 + MAX_ENTRY_NAME + spaces] = 0;
					if(!iec_dir_cache.cbm_medium) {
			            ascii2petscii(&channel->outbuf[7 + spaces], &channel->outbuf[7 + spaces]);
					}
					b = data_strnlen(entry->fname, MAX_ENTRY_NAME);
					if(b > 16) {
						b = 16;
						channel->outbuf[7 + 15 + spaces] = '*'; // wildcard trick for long filenames
						// the bad is....two long filenames with first 15 chars in common 
						// means the first will be loaded. too bad...choose name better ;)
						channel->outbuf[24 + spaces] = ' '; // another patch
					}			
   					channel->outbuf[7 + 16 + spaces] = 32;
		            channel->outbuf[7 + spaces + b] = 34;   // "ABCDEFGHIJKLMNOP"

				    memcpy(&channel->outbuf[25 + spaces], &extsout[entry->ftype*3], 3);		            
		            channel->outbuf[28 + spaces] = (entry->fattrib & AM_RDO) ? '<' : ' ';
		            channel->outbuf[29 + spaces] = 32;
		            channel->outbuf[30 + spaces] = 32;
		            channel->outbuf[31 + spaces] = 0;
		            channel->tx_buffer = &channel->outbuf[2];
		            channel->tx_len    = 30 + spaces;
		            channel->tx_end    = FALSE;
					last_error = ERR_OK;
		            return TRUE;
				}
			}
			else { // No more entries
	            *((WORD*)&channel->outbuf[4]) = 1000;
	            memcpy(&channel->outbuf[6], (BYTE *)dir_end, dir_end_len);		
				memset(&channel->outbuf[18], 0x20, 13);
				channel->outbuf[31] = 0;
				channel->outbuf[32] = 0;
				channel->outbuf[33] = 0;
    	        channel->tx_len    = 34;
        	    channel->tx_end    = TRUE;
				return FALSE;
	        }
		}
	}
		
    return FALSE;
}
/*
-------------------------------------------------------------------------------
							iec_open_file
							=============
  Abstract:

	This function checks whether it can open a file on the requested channel
	and does so if possible. Else, an error code is set. This function does not
	affect the rx/tx pointers.
	
  Parameters
    none. channel should already be set
	
-------------------------------------------------------------------------------
*/
BOOL rw_char(CHAR p, BYTE *acc)
{
    if(p == 'R')
        *acc = ACC_READ;
    else if(p == 'W')
        *acc = ACC_WRITE;
    else if(p == 'A')
        *acc = ACC_WRITE | ACC_APPEND;
    else
        return FALSE;
    
    return TRUE;
}

void iec_open_file()
{
    BYTE b, file;
    BYTE file_type;
    BOOL err;
    BYTE more,plen;
    BYTE access;
    
    printf("(Open %s.%d.%d.%d)", iec_inbuf, cmd, eoi, chan);

	dir_analyze_name(iec_inbuf, MAX_NAME_LEN, &iec_entry_name); // For pattern matching and stuff
    //dump_entryname(&iec_entry_name);
    
    if(!iec_entry_name.len) {
        last_error = ERR_SYNTAX_ERROR_NONAME;
        channel->mode = MODE_ERROR;
        return;
    }

    access     = ACC_READ;
    err        = FALSE;
    file_type  = 1; // default = seq, except when channel <= 1
    channel->rec_len = 0;

    if(chan <= 1) {  // if this is the case, also don't mind the direction parameter, just ignore!
        file_type = 0;
        if(chan == 1) {
            access    = ACC_WRITE;
        } else { // channel = 0
            access    = ACC_READ;
        }    
    } else if(iec_entry_name.param) {
        plen = strlen(iec_entry_name.param);
        more = 1;
        switch(iec_entry_name.param[0]) {
            case 'P':
                file_type = 0;
                break;
            case 'S':
                file_type = 1;
                break;
            case 'U':
                file_type = 2;
                break;
            case 'L':
                file_type = 3;
                if(iec_entry_name.param[1]==',')
                    channel->rec_len = iec_entry_name.param[2];
                if((channel->rec_len == 0xFF)||(!channel->rec_len)) {
                    last_error = ERR_OVERFLOW_IN_RECORD;
                    channel->mode = MODE_ERROR;
                    return;
                }
                access = ACC_READ | ACC_WRITE;
                break;
            default:
                if(rw_char(iec_entry_name.param[0], &access)) {
                    more = 0;
                } else {
                    err = TRUE;
                }
        }
        if(file_type != 3) {
            if((!more)&&(plen > 1)) {
                err = TRUE;
            }
            // now we can expect read or write flags
            if((plen > 1)&&(iec_entry_name.param[1]!=',')) {
                err = TRUE;
            }
            if(plen > 2) {
                if(!rw_char(iec_entry_name.param[2], &access)) {
                    err = TRUE;
                }
            } else {
                err = TRUE;
            }    
        }
    }
    if(err) {
        last_error = ERR_SYNTAX_ERROR_GEN;
        channel->mode = MODE_ERROR;
        return;
    }
    
    file = 0xff;
    // first check if this channel already owns a file
    for(b=0;b<IEC_NUM_FILES;b++) {
        if(filesChan[b] == chan) {
            file = b;
            f_close(&files[b]);
            filesOpen[b] = FALSE;
            break;
        }
    }
    // if not, check if we have files that are not yet open
    if(file == 0xFF) { // no candidate found yet
        for(b=0;b<IEC_NUM_FILES;b++) {
            if(!filesOpen[b]) {
                file = b;
                break;
            }
        }
    }
    // no? then we cant handle this request
    if(file == 0xFF) {
		printf("NO FILE LEFT %d\n", chan);
        last_error = ERR_NO_CHANNEL;
        return;
    }
    // store the link
//    printf("Assigned file #%d to channel %d.\n", file, chan);
    filesChan[file] = chan;
    channel->file = &files[file];
    channel->fileidx = file;
    channel->tx_len = 0;
    channel->tr   = 0;
    channel->mode = MODE_IDLE;

    printf("Open file '%s'. File type = %d, Access type = %d.\n", iec_entry_name.name_str, file_type, access);
    // now actually open the file for read or write
    if(access & ACC_WRITE) {
        strcat(iec_entry_name.name_str, ext_per_type[file_type]);
		iec_entry_name.len += 4; // length of extension added (including .)
        open_for_write(file_type, access); // actually opens file
        printf("Back from open for write.\n");
    } else {
        open_for_read(file_type); // actually opens file  
    }
}


/*
-------------------------------------------------------------------------------
							iec_close_file
							==============
  Abstract:

	This function closes an open file.
	
  Parameters
    none: channel should already be set	
-------------------------------------------------------------------------------
*/
void iec_close_file()
{
    static DWORD dir_cluster;
    
/*
    BYTE b;

    for(b=0;b<NUM_FILES;b++) {
        if(filesChan[b] == chan) {
            filesOpen[b] = FALSE;
            filesChan[b] = 0xFF;
            f_close(&files[b]);
        }
        channel->file = NULL;
    }
*/
    if(filesChan[channel->fileidx] != chan)
        return;

    if(!filesOpen[channel->fileidx]) {
        printf("File is already closed!\n");
        return;
    }
        
    printf("Closing file #%d for channel %d.\n", channel->fileidx, chan);

    filesOpen[channel->fileidx] = FALSE;
    filesChan[channel->fileidx] = 0xFF;
    f_close(&files[channel->fileidx]);
    channel->file = NULL;
    channel->mode = MODE_IDLE; // nothing to transfer
    channel->tr   = 0;
    channel->tx_len = 0;
    channel->rec_len = 0;

	// refresh dir
	if(iec_dir_depth) {
    	dir_cluster = iec_subdir_trace[iec_dir_depth-1];
    } 
	else {
        dir_cluster = 0L;
	}
	iec_dir_change(dir_cluster);
}
