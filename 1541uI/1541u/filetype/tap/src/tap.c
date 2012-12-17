#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tap.h"
#include "c2n.h"
#include "c64_addr.h"
#include "uart.h"

FIL tap_file;

struct t_tap_header
{
    char  id_string[12];
    BYTE  version;
    BYTE  pad[3];
    DWORD length;
};

struct t_tap_header tap_header;

WORD next_block = 0;

BOOL tap_open_file(DIRENTRY *entry)
{
    FRESULT fres;
    WORD br;
    
    f_close(&tap_file);
    C2N_CONTROL = C2N_ERROR; // clear error
    
    fres = f_open_direct(&tap_file, entry);
    if(fres == FR_OK) {
        memset(&tap_header, 0, 20);
        fres = f_read(&tap_file, (void *)&tap_header, 20, &br);
        tap_header.id_string[11]=0;
        if(strncmp(tap_header.id_string, "C64-TAPE-RA", 11)) {
            printf("Not a correct header. %s\n", tap_header.id_string);
            return FALSE;
        }
        printf("TAP Version = %d.\n", tap_header.version);
        next_block = 512 - 20;
    } else {
        printf("Can't open file.\n");
        return FALSE;
    }
    
}

BOOL tap_stream(void)
{
    BYTE st;
    FRESULT fres;
    WORD br;
    
    if(!next_block)
        return FALSE;
        
    st = C2N_STATUS;
    if(!(st & C2N_FIFO_ALMOSTFULL)) {
        fres = f_read(&tap_file, (void *)C2N_FIFO_ADDR, next_block, &br);
        UART_DATA = '>';
        if(fres == FR_OK) {
            if(br) {
                next_block = 512;
            } else {
                next_block = 0; // stop streaming
                return FALSE;
            }
        } else {
            next_block = 0;
            return FALSE;
        }
    } else {
        C2N_CONTROL = C2N_ENABLE;
    }
    return TRUE;
}
