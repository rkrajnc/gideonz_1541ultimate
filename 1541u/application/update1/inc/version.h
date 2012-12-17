#ifndef _VERSION_H
#define _VERSION_H

#include "types.h"

#define SW_VERSION "V1.3\021"
//#define SW_VERSION "V1.1"


#define HW_BASIC    0
#define HW_PLUS     1
#define HW_ETHERNET 2

#define HW_BIT_32MB  0x02
#define HW_BIT_ETH   0x04

#define HW_MSK_BASIC 0x01
#define HW_MSK_PLUS  (HW_MSK_BASIC | HW_BIT_32MB)
#define HW_MSK_ETH   (HW_MSK_PLUS | HW_BIT_ETH)

void version_init(void);

extern BYTE hardware_type;
extern BYTE hardware_mask;

#endif
