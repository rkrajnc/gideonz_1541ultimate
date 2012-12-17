;-------------------------------------------------------------------------------
;--
;--  (C) COPYRIGHT 2007, Gideon's Hardware and Programmable Logic Solutions
;--
;-------------------------------------------------------------------------------
;-- Project    : Ultimate 1541
;-- Title      : gpio.inc
;-------------------------------------------------------------------------------
;-- Revision info:
;--
;--  $Date: $
;--	$Revision: $
;-- 	$Author: $
;-------------------------------------------------------------------------------
;-- Abstract:
;	Definitions allowing for readable use of the io registers
;--
;-------------------------------------------------------------------------------
; addresses
    GPIO_SET    = $2000
    GPIO_CLEAR  = $2001
    GPIO_IN     = $2001
    GPIO_IMASK  = $2002
    GPIO_IPOL   = $2003
    TIMER       = $2004
    GPIO_SET2   = $2006
    GPIO_CLEAR2 = $2007
    GPIO_IN2    = $2007
    IEC_SET     = $200C
    IEC_IN      = $200C
    IEC_CLEAR   = $200D
    SDRAM_CTRL  = $200F

    UART_DATA   = $2100

    TRACK_DIRTY = $2500
    ANY_DIRTY   = $2540

; input 1
    BUTTON0     = $01
    BUTTON1     = $02
    BUTTON2     = $04
    BUTTONS     = $07
    C64_IRQ     = $10

; output 1
    DRIVE_ADDR  = $01
    WRITE_PROT  = $04

; IEC bits (bidir)
    IEC_SRQ     = $08
    IEC_AUTO    = $10
    IEC_ATN     = $20
    IEC_DATA    = $40
    IEC_CLK     = $80
    
; ------------------------------
; output 2
    SOUND_ENABLE = $01
    FLOPPY_INS   = $02
    C64_STOP     = $04
    C64_EXROM    = $08
    C64_GAME     = $10
    C64_NMI      = $20
    DRIVE_RESET  = $40
    C64_RESET    = $80
    
; input 2
    SD_CARDDET   = $01
    SD_WRITEPROT = $02
    
