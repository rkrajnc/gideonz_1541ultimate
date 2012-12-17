;-------------------------------------------------------------------------------
;--
;--  (C) COPYRIGHT 2007, Gideon's Logic Architectures
;--
;-------------------------------------------------------------------------------
;-- Project    : Ultimate 1541
;-- Title      : mem_tools.asm
;-------------------------------------------------------------------------------
;-- Abstract:
;	Fast routines for doing operations on memory.
;--
;-------------------------------------------------------------------------------

	.setcpu		"6502"
	.smart		on
	.autoimport	on
	.case		on
	.debuginfo	off
	.importzp	sp, sreg, regsave, regbank
    
	.export     _ClearFloppyMem
    .export     _SDRAM_Init
    .export     _FillPage
    .export     _TestPage
    .export     _TestLocation
    .export     _memswap
    .export     _copy_page
    .export     SwitchBank
;    .export     _debug_loc1
;    .export     _debug_loc2
;    .export     _stackframe
  	.export		utsteax, tsteax, bankedFp
	.include		"mapper.inc"
    .include        "gpio.inc"
    
    .segment    "ZEROPAGE"
mytmpptr1:  .res 2
mytmpptr2:  .res 2
bankedFp:	.res 2

    .segment    "FASTCODE"
    
_ClearFloppyMem:
	pha
	tya
	pha
	txa
	pha
	
    lda     #WRITE_PROT
    sta     GPIO_SET
    lda     #FLOPPY_INS
    sta     GPIO_CLEAR2

    ; init
	ldx #$00
    stx MAPPER_MAP1H
    stx mytmpptr1
    
outer_loop:
	stx MAPPER_MAP1L
    lda #$40
    sta mytmpptr1+1
    
middle_loop:
    lda #$55

    ldy #0
inner_loop:
    sta (mytmpptr1),y
    iny
    bne inner_loop
    inc mytmpptr1+1
    lda mytmpptr1+1
    cmp #$60
    bne middle_loop
    
    inx
    cpx #40   ; 40 * 8k = 320k
    bne outer_loop
        
    ; finished
    lda     #WRITE_PROT
    sta     GPIO_CLEAR

    ; get registers from stack
    pla
    tax
    pla
    tay
    pla
    rts
    
tsteax:
utsteax:
	tay			; Save value
	stx    	mytmpptr1
	ora	mytmpptr1
	ora	sreg
	ora	sreg+1
	beq	L9
	tya
	ldy	#1  		; Force NE
L9:	rts				  


    ;; this function starts the SDRAM
_SDRAM_Init:
    lda #1
    sta SDRAM_CTRL           ; enable clock
    
    lda #100                  ; wait 500 us
    sta TIMER
    
    lda TIMER
    bne *-3

    lda #>MAP_SDRAM_CMD
    sta MAPPER_MAP1H

    ;; load "precharge all" command address into mapper
    lda #<MAP_SDRAM_CMD + 2  ;  WEn=0, CAS=1, RAS=0
    sta MAPPER_MAP1L

    sta MAP1_ADDR + $0400    ;  issue command

    ;; load "write mode register" command address into mapper
    lda #<MAP_SDRAM_CMD + 0  ;  WEn=0, CAS=0, RAS=0
    sta MAPPER_MAP1L
    
    sta MAP1_ADDR + $0220    ;  Burst length = 1, sequential, single writes, CAS=2
    
    ;; load "auto refresh" command address into mapper
    lda #<MAP_SDRAM_CMD + 4  ;  WEn=1, CAS=0, RAS=0
    sta MAPPER_MAP1L

    sta MAP1_ADDR            ;  issue command
    sta MAP1_ADDR            ;  issue command
    sta MAP1_ADDR            ;  issue command
    sta MAP1_ADDR            ;  issue command
    sta MAP1_ADDR            ;  issue command
    sta MAP1_ADDR            ;  issue command
    sta MAP1_ADDR            ;  issue command
    sta MAP1_ADDR            ;  issue command

    lda #3                   ;  enable refresh
    sta SDRAM_CTRL
    
    lda #0
    sta MAPPER_MAP1H         ; for safety, reset mapper
    rts
    
    .segment    "FASTDATA"

Errors:     .res 2
PageL:      .res 1
PageL_bku:  .res 1

    .segment    "FASTCODE"

_TestPage:
    jsr popax
    sta MAPPER_MAP1L
    stx MAPPER_MAP1H
    
    sta PageL
    txa
    eor PageL
    sta PageL
    sta PageL_bku
        
    lda #$00
    sta mytmpptr1
    sta Errors
    sta Errors+1

    jsr popa
    tax
    beq no_fill

    ;lda #$46
    ;sta $2100
    
    ; start fill
    ; init address and number of pages
    lda #>MAP1_ADDR
    sta mytmpptr1+1
    ldx #$20 ; 8K = 32 pages of 256 bytes
    
    ldy #0
Lp1:tya
    eor PageL
    sta (mytmpptr1),y
    iny
    bne Lp1
    inc mytmpptr1+1
    dec PageL
    dex
    bne Lp1
    
    ; 8K is now filled with data, now test
    lda PageL_bku
    sta PageL

no_fill:
    ; init address and number of pages
    lda #>MAP1_ADDR
    sta mytmpptr1+1
    ldx #$20
    
    ldy #0
Lp2:tya
    eor PageL
    cmp (mytmpptr1),y
    beq DataOk
    inc Errors
    bne DataOk
    inc Errors+1

DataOk:
    iny
    bne Lp2
    inc mytmpptr1+1
    dec PageL
    dex
    bne Lp2
    
    lda Errors
    ldx Errors+1
    rts

    .segment    "FASTCODE"

_FillPage:
    jsr popax
    sta MAPPER_MAP1L
    stx MAPPER_MAP1H
    
    ; start fill
    ; init address and number of pages
    lda #>MAP1_ADDR
    sta mytmpptr1+1
    lda #0
    sta mytmpptr1
    
    ldx #$20 ; 8K = 32 pages of 256 bytes
    
    jsr popa
    ldy #0
Lp7:sta (mytmpptr1),y
    iny
    bne Lp7
    inc mytmpptr1+1
    dex
    bne Lp7
    
    rts

_copy_page:
    jsr popax
    sta MAPPER_MAP2L
    stx MAPPER_MAP2H
    
    jsr popax
    sta MAPPER_MAP1L
    stx MAPPER_MAP1H

    ; start copy
    ; init address and number of pages
    lda #0
    sta mytmpptr1
    sta mytmpptr2

    lda #>MAP1_ADDR
    sta mytmpptr1+1
    lda #>MAP2_ADDR
    sta mytmpptr2+1

    ldx #$20 ; 8K = 32 pages of 256 bytes
    ldy #0
Lp8:lda (mytmpptr1),y
    sta (mytmpptr2),y
    iny
    bne Lp8
    inc mytmpptr1+1
    inc mytmpptr2+1
    dex
    bne Lp8
    rts

    .segment    "CODE"
_TestLocation:
    jsr popa
    sta $3000
    nop
    nop
    nop
    nop
    nop
    cmp $3000
    beq ok
    lda #$04
    sta $2007
    sta $2006
    rts
ok:
    lda #0
    ldx #0    
    rts
    
;    .segment    "BSS"
;_debug_loc1:    .res 2
;_debug_loc2:    .res 2
;_stackframe:    .res 24

    .segment    "FASTCODE"
_memswap:
;    tya
;    pha
;    ldy #0
;Lps:lda (sp),y
;    sta _stackframe,y
;    iny
;    cpy #24
;    bne Lps
;    pla
;    tay
    
    jsr popa
    pha
    jsr popax
    sta mytmpptr1
    stx mytmpptr1+1
    jsr popax
    sta mytmpptr2
    stx mytmpptr2+1
    
;    lda mytmpptr1
;    sta _debug_loc1
;    lda mytmpptr1+1
;    sta _debug_loc1+1
;    lda mytmpptr2
;    sta _debug_loc2
;    lda mytmpptr2+1
;    sta _debug_loc2+1

    pla
    tay
    
Lp: dey
    lda (mytmpptr1),y
    pha
    lda (mytmpptr2),y
    sta (mytmpptr1),y
    pla
    sta (mytmpptr2),y
    cpy #0
    bne Lp

    rts


SwitchBank:
	; switch to bank
;    lda #'['
;    sta $2100
;	lda bankedFp+1
;	jsr uart_hex_asm
;	lda bankedFp
;	jsr uart_hex_asm
;    lda $2102
;    and #$40
;    beq *-5
	lda #$0B
	sta MAPPER_CODE
    lda #>(SwitchBack-1)
    pha
    lda #<(SwitchBack-1)
    pha
	jmp (bankedFp) ; jump to banked address in variable
SwitchBack:
    pha
;    lda #']'
;    sta $2100
;    lda $2102
;    and #$40
;    bne *-5
	; switch back back
	lda #$0F
	sta MAPPER_CODE
    pla
	rts ; jump to original caller
       
