;-------------------------------------------------------------------------------
;--
;--  (C) COPYRIGHT 2007, Gideon's Logic Architectures
;--
;-------------------------------------------------------------------------------
;-- Project    : Ultimate 1541
;-- Title      : <insert file name here>
;-------------------------------------------------------------------------------

	.setcpu		"6502"
    
    .autoimport on
    .importzp   sp, sreg
    .export     _flash_page
    .export     _verify_page
    .include   "mapper.inc"
    .include   "gpio.inc"
	

    .segment    "FASTCODE"
_flash_page:
    jsr popax
    sta MAPPER_MAP2L
    stx MAPPER_MAP2H
    jsr popax
    sta MAPPER_MAP1L
    stx MAPPER_MAP1H

    ; init pointers
    lda #$00
    sta my_src
    sta my_dst
    lda #>MAP1_ADDR
    sta my_src+1
    lda #>MAP2_ADDR
    sta my_dst+1
    
    ldy #0
flp1:
    lda (my_src),y
    cmp #$FF
    beq flp3
    ; Pogram byte command
;    Intel
;    lda #$40
;    sta MAP2_ADDR,y

;    Atmel
    lda #$AA
    sta MAP2_ADDR+$0AAA
    lda #$55
    sta MAP2_ADDR+$1554
    lda #$A0
    sta MAP2_ADDR+$0AAA

    ; write actual bytes
    lda (my_src),y
    sta (my_dst),y
flp2:
;    Intel test
;    bit MAP2_ADDR
;    bpl flp2

;    Atmel test
    lda (my_src),y
    cmp (my_dst),y
    bne flp2

flp3:  
    iny
    bne flp1
    inc my_src+1
    inc my_dst+1
    bpl flp1  ; until my_dst = 0x80
    
    lda #$FF
    sta MAP2_ADDR
    lda #'.'
    sta $2100
    rts

_verify_page:
    jsr popax
    sta MAPPER_MAP2L
    stx MAPPER_MAP2H
    jsr popax
    sta MAPPER_MAP1L
    stx MAPPER_MAP1H

    ; init pointers
    lda #$00
    sta my_src
    sta my_dst
    lda #>MAP1_ADDR
    sta my_src+1
    lda #>MAP2_ADDR
    sta my_dst+1
    
    ldy #0
ver1:
    lda (my_src),y
    cmp (my_dst),y
    bne ver_error
    iny
    bne ver1
    inc my_src+1
    inc my_dst+1
    bpl ver1  ; until my_dst = 0x80
    
    lda #$FF
    sta MAP2_ADDR

    lda #$00
    tax
    rts

ver_error: ; return error address
    tya
    ldx my_dst+1
    rts
    
    .segment    "ZEROPAGE":zeropage
my_src:
    .word       0
my_dst:
    .word       0
   
    