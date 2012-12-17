#include <stdio.h>
#include "types.h"
#include "gpio.h"
#include "c64_addr.h"

const BYTE test_rb[] = { 0xE8, 0xC8, 0x68, 0x48, 0xA8, 0x88, 0x28, 0x08 };

BOOL iec_test(void)
{
    // this routine will toggle the IEC lines from the c64 and see if they arrive on the Ultimate
    BYTE i,b;

    CIA2_DDRA = 0x3C;

    for(i=0;i<7;i++) {
        CIA2_DPA = (i << 3);
        TIMER = 10;
        while(TIMER)
            ;
        b = IEC_IN;
        if(b == test_rb[i]) {
            printf("OK ");
        } else {
            printf("Not OK! (%02x %02x)\n", b, test_rb[i]);
            return FALSE;
        }
    }
}
