#include <stdio.h>
#include "version.h"
#include "onewire.h"

BYTE hardware_type;
BYTE hardware_mask;
BYTE serial[8];

void version_init(void)
{
    BYTE b;

    hardware_type = HW_BASIC;
    hardware_mask = HW_MSK_BASIC;
    
    if(onewire_readrom(serial)) {
        for(b=0;b<8;b++) {
            printf("%02X ", serial[b]);
        }
        b = onewire_readmem(0x60);
        if(b == 0xFF) {
            hardware_type = HW_PLUS;
            hardware_mask = HW_MSK_PLUS;
        } else if(b == 0x7F) {
            hardware_type = HW_ETHERNET;
            hardware_mask = HW_MSK_ETH;
        }
    } else {
        printf("Onewire device not present.\n");
    }
}
