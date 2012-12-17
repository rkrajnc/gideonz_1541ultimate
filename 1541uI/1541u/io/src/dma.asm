;-------------------------------------------------------------------------------
;--
;--  (C) COPYRIGHT 2007, Gideon's Logic Architectures
;--
;-------------------------------------------------------------------------------
;-- Project    : Ultimate 1541
;-- Title      : 64_dma.asm
;-------------------------------------------------------------------------------
;-- Abstract:
;-- Software routines needed for loading / saving programs through DMA
;--
;-------------------------------------------------------------------------------

	.setcpu		"6502"
	.smart		on
	.autoimport	on
	.case		on
	.debuginfo	off
;	.importzp	sp, sreg, regsave, regbank, tmp1, ptr1, ptr2
    
	.export		_dmacode_start
	.export     _dmacode_end
    .include    "mapper.inc"
    	
.segment    "RODATA"

_dmacode_start:
    .word 0     ; vector backup
    .byte 0     ; handshake_in
    .byte 0     ; handshake_out
    .word 0     ; sid_start
    
    vector_backup = $0140
    handshake_in  = $0142
    handshake_out = $0143
    sid_start     = $0144
    irq_vector    = $0314
    
code:  ;; IRQ!
    pha

    lda #$FF
    sta handshake_out
    lda #$30 ; show ram!
    sta $01
    bit handshake_in
    bpl *-3
    lda #$37 ; restore roms and I/O
    sta $01
    lda vector_backup
    sta irq_vector
    lda vector_backup+1
    sta irq_vector+1
    lda #$00
    sta handshake_out

    bit handshake_in
    bvc do_run
    lda handshake_in
    lsr
    bcs do_run_rsid
    
    pla
    jmp (vector_backup)
do_run:

    pla ; our own stack push
    pla ; 3x from the interrupt
    pla
    pla

    cli ; we just let the interrupt occur again

    lda #1   ; disable cursor blink
    sta $CC
    lda #'R'
    sta $0200
    lda #'U'
    sta $0201
    lda #'N'
    sta $0202
    lda #0
    sta $13
    ldx #3
    jsr $AACA
    jmp $A486    

do_run_rsid:
    pla ; our own stack push
    pla ; 3x from the interrupt
    pla
    pla
    cli
    
    lda $02
    sta $d020
    ldx #0
    ldy #0
    iny
    bne *-1
    inx
    bne *-4

    ldx $02
    dex
    txa
    tay
    jmp (sid_start)

_dmacode_end:
 
    nop
    
;.segment    "FASTCODE"
;
;_dmatest:
;    lda #<MAP_SCR
;    sta MAPPER_MAP1L
;    lda #>MAP_SCR
;    sta MAPPER_MAP1H
;    
;    ldy #0
;testloop:
;    sty $4400
;    cpy $4400
;    bne error
;    iny
;    jmp testloop
;error:
;    inc $4401
;    rts
;    