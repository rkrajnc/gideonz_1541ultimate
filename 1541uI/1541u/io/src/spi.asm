;-------------------------------------------------------------------------------
;--
;--  (C) COPYRIGHT 2007, Gideon's Logic Architectures
;--
;-------------------------------------------------------------------------------
;-- Project    : Ultimate 1541
;-- Title      : spi.asm
;-------------------------------------------------------------------------------
;-- Abstract:
;	Direct control of the SPI interface.
;--
;-------------------------------------------------------------------------------

	.setcpu		"6502"
	.smart		on
	.autoimport	on
	.case		on
	.debuginfo	off
	.importzp	sp, sreg, regsave, regbank, tmp1, ptr1, ptr2
    
	.export		_spi_Init
	.export     _spi_Send
	.export     _spi_Clocks
	.export     _spi_SetSpeed
    .export     _spi_ReadBlockFast
    .export     _spi_WriteBlockFast
	.export		_sd_Command
	.export     _uart_hex
	.export     uart_hex_asm

    .include   "gpio.inc"
	
	SPI_DATA	 =	 $2200
	SPI_SPEED	 =	 $2201
	SPI_CTRL	 =	 $2202
	SPI_CRC	     =	 $2203

;	SPI_CPOL	 =	 $01
;	SPI_CPHA	 =	 $02
	SPI_BUSY	 =	 $80

	SPI_FORCE_SS =	 $01
	SPI_LEVEL_SS =	 $02

    .segment    "FASTCODE"

;void if_spiInit(hwInterface *iface)
_spi_Init:
;	// low speed during init
	lda #$FE
	sta SPI_SPEED
	
;	/* Send 20 spi commands with card not selected */
    lda #100
    jsr pusha
    jmp _spi_Clocks

_spi_Clocks:
	lda #SPI_FORCE_SS + SPI_LEVEL_SS
	sta SPI_CTRL

    jsr popa
    tax
    lda #$FF
scloop:
	sta SPI_DATA
    bit SPI_CTRL
    bmi *-3
    dex
    bne scloop

	lda #0
	sta SPI_CTRL
	tax
	rts

_sd_Command:
;void sd_Command(hwInterface *iface,euint8 cmd, euint16 paramx, euint16 paramy)
;{
;	if_spiSend(iface,0xff);
;
;	if_spiSend(iface,0x40 | cmd);
;	if_spiSend(iface,(euint8) (paramx >> 8)); /* MSB of parameter x */
;	if_spiSend(iface,(euint8) (paramx)); /* LSB of parameter x */
;	if_spiSend(iface,(euint8) (paramy >> 8)); /* MSB of parameter y */
;	if_spiSend(iface,(euint8) (paramy)); /* LSB of parameter y */
;
;	if_spiSend(iface,0x95); /* Checksum (should be only valid for first command (0) */
;
;	if_spiSend(iface,0xff); /* eat empty command - response */
;}
	lda #$FF
	sta SPI_DATA
	bit SPI_CTRL
	bmi *-3

	jsr popax
	pha ; low y
	txa
	pha ; high y
	jsr popax
	pha ; low x
	txa
	pha ; high x
	jsr popa
	ora #$40
    sta SPI_CRC
	sta SPI_DATA
	bit SPI_CTRL
	bmi *-3
	pla
	sta SPI_DATA
	bit SPI_CTRL
	bmi *-3
	pla
	sta SPI_DATA
	bit SPI_CTRL
	bmi *-3
	pla
	sta SPI_DATA
	bit SPI_CTRL
	bmi *-3
	pla
	sta SPI_DATA
	bit SPI_CTRL
	bmi *-3
;	lda #$95
    lda SPI_CRC
	sta SPI_DATA
	bit SPI_CTRL
	bmi *-3
;	lda #$FF
;	sta SPI_DATA
;	bit SPI_CTRL
;	bmi *-3
	lda #$00
	tax
	rts

_spi_ReadBlockFast:
    tya
    pha
    
    jsr popax  ; fetch pointer
    sta tmp1   ; store pointer
    stx tmp1+1 ; store pointer

    ldy #0
    ldx #2
         
looprbf1:
    lda #$ff
    sta SPI_DATA
	bit SPI_CTRL
	bmi *-3
    ; 18 clocks
    lda SPI_DATA
    sta (tmp1),y
    iny
    bne looprbf1
    inc tmp1+1
    dex
    bne looprbf1
    
    pla
    tay
    lda #0
    tax
    rts
    
_spi_WriteBlockFast:
;	spi_Send(0xfe); // Start block 
;	for(i=0;i<512;i++) 
;		spi_Send(buf[i]); // Send data 
;	spi_Send(0xff); // Checksum part 1 
;	spi_Send(0xff); // Checksum part 2 
;	spi_Send(0xff);
;
;   while(spi_Send(0xff)!=0xff){
;		t++;
;		// Removed NOP 
;	}
;    return t;
;
    lda #$ff
    ldx #10
wbf0:
    sta SPI_DATA    
    bit SPI_CTRL
    bmi *-3
    dex
    bne wbf0

    lda #$fe
    sta SPI_DATA
    bit SPI_CTRL
    bmi *-3
    
    jsr popax  ; fetch pointer
    sta tmp1   ; store pointer
    stx tmp1+1 ; store pointer

    ldy #0
    ldx #2
         
loopwbf1:
    lda (tmp1),y
    sta SPI_DATA
    bit SPI_CTRL
    bmi *-3
    iny
    bne loopwbf1
    inc tmp1+1
    dex
    bne loopwbf1

    lda #$ff
    ldy #3
loopwbf2:
    sta SPI_DATA
    bit SPI_CTRL
    bmi *-3
    dey
    bne loopwbf2

    ldx #0
    ldy #0
    
loopwbf3:
    lda #$FF
    sta SPI_DATA
    bit SPI_CTRL
    bmi *-3
    lda SPI_DATA
    cmp #$FF
    beq wbfdone
    iny
    bne loopwbf3
    inx
    jmp loopwbf3

wbfdone:
    tya  ; low byte counter, x = high byte
    rts
                
_spi_Send:
	jsr popa
	sta SPI_DATA
	bit SPI_CTRL
	bmi *-3
	lda SPI_DATA
	ldx #$00
	rts

_spi_SetSpeed:
	jsr popa
	sta SPI_SPEED
    rts


    .segment    "FASTDATA"
hex:    .byte "0123456789ABCDEF"

    .segment    "FASTCODE"
_uart_hex:
    jsr popa
    sta tmp1
    lsr
    lsr
    lsr
    lsr
    tax
    lda hex,x
    sta UART_DATA
    lda tmp1
    and #$0f
    tax
    lda hex,x
    sta UART_DATA
;    lda #$20
;    sta UART_DATA
    rts

uart_hex_asm:
	pha
    lsr
    lsr
    lsr
    lsr
    tax
    lda hex,x
    sta UART_DATA
	pla
    and #$0f
    tax
    lda hex,x
    sta UART_DATA
    rts

;    .segment    "WORK":zeropage

;    .segment    "DATA"
    
