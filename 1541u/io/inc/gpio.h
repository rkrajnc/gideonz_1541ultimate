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
 *	 GPIO definitions representing their hardware wiring
 */

#ifndef GPIO_H
#define GPIO_H
	

#define GPIO_SET     *((BYTE *)0x2000) // write
#define GPIO_CLEAR   *((BYTE *)0x2001) // read
#define GPIO_OUT     *((BYTE *)0x2000) // write
#define GPIO_IN      *((BYTE *)0x2001) // read
#define GPIO_IMASK   *((BYTE *)0x2002)
#define GPIO_IPOL    *((BYTE *)0x2003)
#define TIMER        *((BYTE *)0x2004)
#define CART_MODE_RB *((BYTE *)0x2005) // read
#define CART_KILL    *((BYTE *)0x2005) // write
#define GPIO_SET2    *((BYTE *)0x2006)
#define GPIO_IN2     *((BYTE *)0x2007)
#define GPIO_CLEAR2  *((BYTE *)0x2007)
#define IRQ_TIMER    *((WORD *)0x2008)
#define IRQ_TMR_CTRL *((WORD *)0x200A)
#define DEBUG1       *((BYTE *)0x200B) // write
#define IEC_SET      *((BYTE *)0x200C) // write
#define IEC_IN       *((BYTE *)0x200C) // read
#define IEC_CLEAR    *((BYTE *)0x200D) // write
#define DEBUG2       *((BYTE *)0x200E) // write
#define SDRAM_CTRL   *((BYTE *)0x200F) // write

/* Configuration */
#define CONFIG_BUTTONS  *((BYTE *)0x2900) // write only
#define CONFIG_ETH_EN   *((BYTE *)0x2901) // write only
#define CART_MODE       *((BYTE *)0x2902) // write only
#define CART_BASE       *((BYTE *)0x2903) // write only
#define CONFIG_RAMBO_EN *((BYTE *)0x2904) // write only

#define IEC_DATA     *((BYTE *)0x2800) // read/write
#define IEC_CTRL   	 *((BYTE *)0x2801) // write
#define IEC_STAT 	 *((BYTE *)0x2801) // read
#define IEC_ADDR     *((BYTE *)0x2802) // write only
//#define IEC_STATE    *((BYTE *)0x2803) // read

#define TRACEDBG(a)	DEBUG1=a

/* GPIO IN register */

#define BUTTON0       0x01
#define BUTTON1       0x02
#define BUTTON2       0x04
#define BUTTONS       0x07
#define C64_IRQ       0x10
#define ONE_WIRE      0x40
#define C64_CLOCK_DET 0x80 

/* GPIO IEC bits */
#if 0
#define IEC_SRQ      0x08
#define IEC_AUTO     0x10
#define IEC_ATN      0x20
#define IEC_DATA     0x40
#define IEC_CLK      0x80
#endif

#define IEC_CTRL_READY	  0x80
#define IEC_CTRL_NODATA   0x40
#define IEC_CTRL_TALKER   0x20
#define IEC_CTRL_LISTENER 0x10
#define IEC_CTRL_DATA_AV  0x08
#define IEC_CTRL_SWRESET  0x04
#define IEC_CTRL_KILL     0x02
#define IEC_CTRL_EOI      0x01

#define IEC_STAT_DATA_AV     0x80
#define IEC_STAT_TALKER      0x20
#define IEC_STAT_LISTENER    0x10
#define IEC_STAT_CLR_TO_SEND 0x08
#define IEC_STAT_BYTE_SENT   0x04
#define IEC_STAT_ATN         0x02
#define IEC_STAT_EOI         0x01

/* GPIO Out register 1 */
/*   7       6       5       4       3       2       1       0
   MASK_B  ONEWIRE IEC_ERR [STOPCONDITION] WRPROT [DRIVE_ADDRESS] */
   
#define DRIVE_ADDR_BIT  0
#define DRIVE_ADDR_MASK 0x03
#define WRITE_PROT      0x04
#define STOP_COND_BIT   3
#define STOP_COND       0x18
#define IEC_IF_ERROR	0x20
#define MASK_BUTTONS    0x80

/* GPIO Out register 2 */
#define SOUND_ENABLE  0x01
#define FLOPPY_INS    0x02
#define C64_STOP      0x04
#define C64_EXROM     0x08
#define C64_GAME      0x10
#define C64_NMI       0x20
#define DRIVE_RESET   0x40
#define FPGA_RECONFIG 0x80 // boot
#define C64_RESET     0x80 // application 

/* GPIO In register 2 */
#define SD_CARDDET   0x01
#define SD_WRITEPROT 0x02
 
/* IRQ Timer Control */
#define TMR_ENABLE  0x01
#define TMR_DISABLE 0x02
#define TMR_IRQ     0x04


#define DELAY5US(a)	TIMER=a;while(TIMER)

#endif
