;-------------------------------------------------------------------------------
;--
;--  (C) COPYRIGHT 2007, Gideon's Hardware and Programmable Logic Solutions
;--
;-------------------------------------------------------------------------------
;-- Project    : Ultimate 1541
;-- Title      : <insert file name here>
;-------------------------------------------------------------------------------
;-- Revision info:
;--
;--  $Date: $
;--	$Revision: $
;-- 	$Author: $
;-------------------------------------------------------------------------------
;-- Abstract:
;	<insert UNDERSTANDABLE description here>
;--
;-------------------------------------------------------------------------------

	.setcpu		"6502"
    
;    .word   *+2     ; load address
    .importzp   sp, sreg
	.import		_main
    .import     __DATA_LOAD__
    .import     __DATA_RUN__
    .import     __DATA_SIZE__
    .import     __FASTDATA_LOAD__
    .import     __FASTDATA_RUN__
    .import     __FASTDATA_SIZE__
    .import     __BSS_LOAD__
    .import     __BSS_RUN__
    .import     __BSS_SIZE__
    .import     __STACK_START__
    .import     __STACK_SIZE__
    .export     _get_sp
    .export     _get_sreg
    .import     _SDRAM_Init
    
    .include   "mapper.inc"
    .include   "gpio.inc"
        
    .segment    "ZEROPAGE":zeropage
    .res        6,$00 ; to make sure we don't get RAM warnings during simulation (trace buffer violation)
my_src:
    .word       0
my_dst:
    .word       0

    .segment    "STARTUP"
start:
	jmp startup

nmi_vector:
	jmp nmi_handler
	
startup:
    sei
    ldx #$ff
    txs

    lda #$41
    sta $2100
    lda #$3A
    sta $2100

    ; Copy Fast Code page from SRAM to internal BRAM (to avoid the need for data2mem! (open source issue))
    ; Fastpage is located at bus address 60000, and needs to be copied to $1000-$1FFF.
    ; This needs to take place only once, since the region 60000 is later used for backup of freeze.
    ; Bus address 60000 translates to map #$0030, offset $0000
    
    lda $1000
    bne fastcode_done
    
    lda #$30
    sta MAPPER_MAP1L
    lda #$00
    sta MAPPER_MAP1H
    lda #<(MAP1_ADDR + $0000)
    sta my_src
    lda #>(MAP1_ADDR + $0000)
    sta my_src+1
    lda #0
    sta my_dst
    lda #$10
    sta my_dst+1
    ldx #$10
    ldy #0
loop_fastcode:
    lda (my_src),y
    sta (my_dst),y
    iny
    bne loop_fastcode
    inc my_src+1
    inc my_dst+1
    dex
    bne loop_fastcode
fastcode_done:
    lda #$42
    sta $2100
    lda #$3A
    sta $2100

    jmp init_segments


    .segment    "FASTCODE"
init_segments:
    lda #$43
    sta $2100
    lda #$3A
    sta $2100

    ; Copy data load to data run
    lda #<__DATA_LOAD__
    sta my_src
    lda #>__DATA_LOAD__
    sta my_src+1
    
    lda #<__DATA_RUN__
    sta my_dst
    lda #>__DATA_RUN__
    sta my_dst+1
    
    lda #>__DATA_SIZE__
    ora #<__DATA_SIZE__
    beq donedata

    ldy #0 ; default block = 256
    ldx #>__DATA_SIZE__

startdata:
    cpx #$00     ; last block?
    bne loopdata ; no? (>= $100 bytes)
    ldy #<__DATA_SIZE__  ; yes! < $100 bytes

loopdata:
    dey
    lda (my_src),y
    sta (my_dst),y
    cpy #$00
    bne loopdata

    inc my_src+1
    inc my_dst+1
    cpx #$00
    beq donedata
    dex
    jmp startdata

donedata:
    lda #$42
    sta $2100
    lda #$3A
    sta $2100

    ; Copy fast load to fast run
    lda #<__FASTDATA_LOAD__
    sta my_src
    lda #>__FASTDATA_LOAD__
    sta my_src+1
    
    lda #<__FASTDATA_RUN__
    sta my_dst
    lda #>__FASTDATA_RUN__
    sta my_dst+1
    
    lda #>__FASTDATA_SIZE__
    ora #<__FASTDATA_SIZE__
    beq donefastdata

    ldy #0 ; default block = 256
    ldx #>__FASTDATA_SIZE__

startfastdata:
    cpx #$00     ; last block?
    bne loopfastdata ; no? (>= $100 bytes)
    ldy #<__FASTDATA_SIZE__  ; yes! < $100 bytes

loopfastdata:
    dey
    lda (my_src),y
    sta (my_dst),y
    cpy #$00
    bne loopfastdata

    inc my_src+1
    inc my_dst+1
    cpx #$00
    beq donefastdata
    dex
    jmp startfastdata

donefastdata:
    lda #$3A
    sta $2100
    lda #$44
    sta $2100

    ; Clear BSS
    lda #<__BSS_RUN__
    sta my_dst
    lda #>__BSS_RUN__
    sta my_dst+1
    
    lda #>__BSS_SIZE__
    ora #<__BSS_SIZE__
    beq donebss

    lda #0

    ldy #0 ; default block = 256
    ldx #>__BSS_SIZE__

startbss:
    cpx #$00     ; last block?
    bne loopbss ; no? (>= $100 bytes)
    ldy #<__BSS_SIZE__  ; yes! < $100 bytes

loopbss:
    dey
    sta (my_dst),y
    cpy #$00
    bne loopbss

    inc my_dst+1
    cpx #$00
    beq donebss
    dex
    jmp startbss

donebss:
    lda #>(__STACK_START__ + __STACK_SIZE__ - 1)
    sta sp+1;
    lda #<(__STACK_START__ + __STACK_SIZE__ - 1)
    sta sp

    lda #0 ; turn off all interrupts
    sta GPIO_IMASK

    ; TODO!
    lda #'<'
    sta $2100
    jsr _SDRAM_Init
    lda #'>'
    sta $2100
    lda $2306
    clc
    adc #$40
    sta $2100

    cli    
    jmp _main
        
_get_sp:
    lda sp
    ldx sp+1
    rts

_get_sreg:
    lda sreg
    ldx sreg+1
    rts

nmi_handler:
	rti

    .segment    "FASTCODE"
irq_vector:
    rti
    
    .segment    "VECTORS"
    .word       $FFFF  ; don't change this to 0, otherwise the startup code will always copy the fast code segment
    .word       $FFFF
    .word       $FFFF
    .word       $FFFF
    .word       $FFFF
    .word       nmi_vector
    .word       start
    .word       irq_vector
    