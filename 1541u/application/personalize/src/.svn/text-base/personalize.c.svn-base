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
#include "personalize.h"

extern FIL     prog_file; // Using the programmer file pointer to reduce mem consumption
extern FRESULT prog_fres; // Using the programmer result to reduce mem consumption
extern char owner_name[32];

// This file personalizes a card, and traces the serial number, storing it on the SD-card.

WORD read_index(void)
{
    static char buffer[16];
    static WORD idx;
    static WORD bytes_read;
        
    prog_fres = f_open(&prog_file, "index.txt", FA_READ);
    memset(buffer, 0, 16);    
    if(prog_fres == FR_OK) {
        prog_fres = f_read(&prog_file, buffer, 16, &bytes_read);
        f_close(&prog_file);
        sscanf(buffer, "%d", &idx);
        printf("Index file read. Index = %d.\n", idx);
        return idx;
    }
    return 0xFFFF;
}

BOOL write_index(WORD index)
{
    static char buffer[16];
    static WORD bytes_written;
    
    prog_fres = f_open(&prog_file, "index.txt", FA_WRITE|FA_CREATE_ALWAYS);
    sprintf(buffer, "%d", index);
    if(prog_fres == FR_OK) {
        prog_fres = f_write(&prog_file, buffer, strlen(buffer), &bytes_written);
        printf("Index file updated.\n");
        f_close(&prog_file);
        return TRUE;
    }
    return FALSE;
}

int read_line(FIL *my_file, char *line, int maxlen)
{
    WORD w;
    WORD idx;

    char *start = line;
    memset(line, 0, maxlen);
    idx = 0;
    do {
        f_read(my_file, line, 1, &w);
        idx++;
    } while((*(line++) != 0x0a)&&(w)&&(idx < maxlen));
    
    *(--line) = 0;
    
    return idx;  
}

struct user_info *decode_user(char *line)
{
    static struct user_info my_user;
    BYTE b, quote, stop;
    char **user;
    char *column, *pntr, *next;

    user = (char **)&my_user; // overlay char pointer array

//    printf("Line = '%s'\n", line);

    column = line;
    stop = 0;
    for(b=0;(b<NUM_FIELDS)&&(!stop);b++) {
        pntr = column;
        quote = 0;
        // search end of column
        while(1) {
            if(!(*pntr)) {
                stop = 1;
                break;
            }
            if(*pntr == 0x22) {
                quote ^= 1;
                *pntr = ' ';
            }
            if(*pntr == 0xA0) {
                *pntr = ' ';
            }
            if((*pntr == ',')&&(!quote))
                break;
            pntr++;
        }
        *pntr = 0; // cut string
//        printf("Column = '%s'\n", column);
        next = pntr + 1;

        // trim leading spaces
        while(*column == ' ')
            column++;

        // trim closing spaces
        while(pntr[-1] == ' ') {
            pntr--;
            *pntr = 0;
        }
        // store string pointer
        user[b] = column;
        // start next column after ,
        column = next;   
    }        
    // debug
    printf("Type: %s\nName: %s %s\nAddress:\n %s\n %s\n %s\n %s\n\n", my_user.type,
        my_user.firstname, my_user.lastname,
        my_user.address1, my_user.address2,
        my_user.address3, my_user.address4 );

    return &my_user;
}

struct user_info *get_user(WORD index)
{
    static char linebuffer[1000];
    WORD i, len;
    
    prog_fres = f_open(&prog_file, "orders.csv", FA_READ);
    if(prog_fres == FR_OK) {
        index++;
        for(i=0;i<=index;i++) {
            len = read_line(&prog_file, linebuffer, 1000);
            if(!len) {
                printf("read error. end of file?\n");
                return NULL;
            }
        }
        f_close(&prog_file);
    } else {
        printf("Couldn't open orders file.\n");
    }
    return decode_user(linebuffer);
}

BOOL log_serial(BYTE *serial, struct user_info *user)
{
    static FILINFO my_info;
    static char buffer[200];
    static WORD bw;
        
//    sprintf(buffer,"%02X%02X%02X%02X %02X%02X%02X%02X %s %s %s\n", serial[3], serial[2], serial[1], serial[0],
//            serial[7], serial[6], serial[5], serial[4], user->firstname, user->lastname, user->type);

    sprintf(buffer, "INSERT INTO `boards` ( `OrderRef`, `Serial`, `Type` ) VALUES ( '%s', '%02X%02X%02X%02X %02X%02X%02X%02X', '%s' );\n",
        user->id, serial[3], serial[2], serial[1], serial[0], serial[7], serial[6], serial[5], serial[4], user->type );

    prog_fres = f_stat("serials.log", &my_info);
    if(prog_fres == FR_OK) {
        prog_fres = f_open(&prog_file, "serials.log", FA_WRITE);
        if(prog_fres == FR_OK) {
//            printf("Length of file = %ld.\n", my_info.fsize);
            // move to end = append
            prog_fres = f_lseek(&prog_file, my_info.fsize);
//            printf("lseek = %d.\n", prog_fres);
            f_write(&prog_file, buffer, strlen(buffer), &bw);
        } else {
            printf("Couldn't get file stats.\n");
            f_close(&prog_file);
            return FALSE;
        }
        f_close(&prog_file);
    } else {
        printf("Can't open serials log file.\n");
        return FALSE;
    }
    return TRUE;
}

BOOL log_status(struct user_info *user)
{
    static FILINFO my_info;
    static char buffer[100];
    static WORD bw;
        
    sprintf(buffer,"UPDATE `orders` SET `Status` = 'Ready to ship' WHERE `ID` = %s LIMIT 1;\n", user->id);
            
    prog_fres = f_stat("status.sql", &my_info);
    if(prog_fres == FR_OK) {
        prog_fres = f_open(&prog_file, "status.sql", FA_WRITE);
        if(prog_fres == FR_OK) {
//            printf("Length of file = %ld.\n", my_info.fsize);
            // move to end = append
            prog_fres = f_lseek(&prog_file, my_info.fsize);
//            printf("lseek = %d.\n", prog_fres);
            f_write(&prog_file, buffer, strlen(buffer), &bw);
        } else {
            printf("Couldn't get file stats.\n");
            f_close(&prog_file);
            return FALSE;
        }
        f_close(&prog_file);
    } else {
        printf("Can't open status SQL file.\n");
        return FALSE;
    }
    return TRUE;
}


BOOL store_name_in_ee(char *name)
{
    char buffer[32];
    BYTE offset;
    
    memset(buffer, 0xFF, 32);
    strncpy(buffer, name, 32);

    // copy these 32 bytes into the E2prom, using four 8 byte transfers.
    for(offset = 0;offset<32;offset += 8) {    
        if(!onewire_progbuf(offset, &buffer[offset], 8))
            return FALSE;
    }
    return TRUE;
}

BOOL set_version(char *type)
{
    char aa = 0xAA;
    char zeros[2] = { 0x00, 0x00 };
    char eth[2]   = { 0x7F, 0xFF };
    char bootversion = 0x16;
    
    char ffs[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE };
    BYTE readbuf;
    
    onewire_reset();
    onewire_sendbyte(0xCC); // skip rom
    onewire_sendbyte(0xF0); // read memory
    onewire_sendbyte(0x83); // starting address
    onewire_sendbyte(0);    // high byte
    readbuf = onewire_getbyte();
    
    printf("Programming Ultimate '%s'!\n", type);
    if(readbuf != 0xAA) {
        onewire_progbuf(0x60, ffs, 8);
        printf("Setting page 3 to EPROM mode.\n");
        if(!onewire_progbuf(0x83, &aa, 1)) {
            printf("Couldn't set page 3 to EPROM mode.\n");
            return FALSE;
        }
    }

    // set bootloader version
    onewire_progbuf(0x5F, &bootversion, 1);

    if(type[0]=='e') {
        onewire_progbuf(0x62, &aa, 1);
        return onewire_progbuf(0x60, &eth[0], 2);  
    } else if(type[0]=='b') {
        return onewire_progbuf(0x60, &zeros[0], 2);
    } else if(type[0]='p') {
        return onewire_progbuf(0x61, &zeros[0], 1);
    } else {
        printf("Unknown type!\n");
    }
    return FALSE;
}

BOOL personalize(struct user_info *user)
{
    BOOL log;
    BYTE b;
        
    static BYTE serial[8];
    char fullname[40];

    // get serial number
    printf("Serial: ");
    if(onewire_readrom(serial)) {
        for(b=0;b<8;b++) {
            printf("%02X ", serial[b]);
        } printf("\n");
    } else {
        printf("Onewire device not present.\n");
        return FALSE;
    }

    // log serial # to file
    log = log_serial(serial, user);
    if(!log) {
        printf("Serial # logging failed.\n");
        return FALSE;
    }
    
    // log update sql to file
    log = log_status(user);
    if(!log) {
        printf("Status logging failed.\n");
        return FALSE;
    }

    sprintf(fullname, "%s %s", user->firstname, user->lastname);
    printf("Storing name '%s' into E2PROM.\n", fullname);
    if(!store_name_in_ee(fullname))
        return FALSE;

    printf("Storing name in flash, too...\n");
    // store name in flash, too
    cfg_load_defaults(0xFF);
    cfg_load_flash();
    strcpy(owner_name, fullname);
    cfg_save_flash();
    
    if(!set_version(user->type))  {
        return FALSE;
    }

/*
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
*/
    return TRUE;
}
