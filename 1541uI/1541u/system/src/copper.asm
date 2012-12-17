	.setcpu		"6502"
	.smart		on
	.autoimport	on
    .export     _copper_list
    .export     _copper_pntr
    .export     _copper_en
    .export     _soft_irq
    .export     _handle_vic_irq
    .include   "mapper.inc"
    .include   "gpio.inc"

    COP_POS_Y1     = 5
    COP_POS_Y2     = 11
    COP_POS_SCROLL = 19


    .segment "FASTDATA"

_copper_list: .res 32,$FF
_copper_pntr: .byte $00
_copper_en:   .byte $00
_soft_irq:    .byte $00

    .segment    "FASTCODE"

.proc _handle_vic_irq
    bit MAP3_ADDR+25 ;VIC_IREG
    bpl irq_done
    bit _copper_en
    bpl copper_done
    ldx _copper_pntr
next_move:
    lda _copper_list,x
    cmp #$FF
    bne list_ok
    sta _soft_irq
    ldx #0
    lda _copper_list,x
list_ok:   
    tay
    inx
    lda _copper_list,x
    sta MAP3_ADDR,y    
    inx
    cpy #$12 ; D012 (new raster line setting)    
    bne next_move
    stx _copper_pntr
copper_done:
    lda #$8F         ;Clear Raster IRQ
    sta MAP3_ADDR+25 ;VIC_IREG
irq_done:
    rts         ; if we got here because bpl, N=0, in other cases N=1, because lda #$8F 


.endproc

.segment "LIBCODE"
.proc _do_scroller

    .export     _new_message
    .export     _scroll_msg
    .export     _do_scroller

.segment "DATA"
scroll:     .byte $00
pause:      .byte $00
new_char:   .byte $00
speed:      .byte $01

_new_message:   .byte $00
_scroll_msg:    .addr _default_msg

.segment "ZEROPAGE"
pointer:    .res 2

.segment "LIBCODE"

;    b = (BYTE)((index - offset) << 3);
;    copper_list[COP_POS_Y1] = b + 0x42;
;    copper_list[COP_POS_Y2] = b + 0x4A;

    lda _menu_index
    sec
    sbc _menu_offset
    asl
    asl
    asl
    clc
    adc #$42
    sta _copper_list + COP_POS_Y1
    adc #$08
    sta _copper_list + COP_POS_Y2
    
;    if(scroll_enable)
    lda _scroll_enable
    bne start_scroll
    rts
    
;    if(new_message) {
;        new_message --;
;        if(!new_message) {
;            message = scroll_msg;
;            pause = 0;
;            pos = 0;
;            speed = 1;

start_scroll:
    lda _new_message
    beq no_new
    tax
    dex
    stx _new_message
    bne old_out
    lda _scroll_msg
    sta pointer
    lda _scroll_msg+1
    sta pointer+1
    ldx #0
    stx pause
    inx
    stx speed
    bne no_new ; always jump     

old_out:
;        } else {
;            scroll_char = ' ';
;            scroll_left();
;            scroll_left();
;            return;
;        }
;    }
    lda #$20
    sta _scroll_char
    jsr _scroll_left
    jmp _scroll_left

no_new:
;    if(pause) {
;        pause--;
;        return;
;    }
    ldx pause
    beq no_pause
    dex
    stx pause
    rts

no_pause:
;    scroll += speed;
;    copper_list[COP_POS_SCROLL] = (scroll & 0x07) ^ 0x07;

    lda scroll
    clc
    adc speed
    sta scroll
    and #$07
    tay
    eor #$07
    sta _copper_list + COP_POS_SCROLL

;    if(scroll & 0x08) { // wrap around
;        scroll &= 0x07;
    
    lda scroll
    sty scroll ; scroll and 7
    
    and #$08
    bne loop
    rts
        
loop:
;        // determine what to do
;        do {
;            new_char = message[pos++];

    ldy #0
    lda (pointer),y
    sta new_char
    inc pointer
    bne pntr_ok
    inc pointer+1
pntr_ok:
;            if(!new_char) {
;                pos = 0;
    
    lda new_char
    bne not_end
    lda _scroll_msg
    sta pointer
    lda _scroll_msg+1
    sta pointer+1
    jmp loop
;            } else if(new_char < 6) {
;                speed = (BYTE)new_char - 1; // /001 = stop, /002 = slow, /003 = medium, /004 = fast
not_end:
    cmp #6
    bcs gte6
    tax
    dex
    stx speed
    jmp loop
gte6:
;            } else if(new_char == 8) {
;                pause = (BYTE)message[pos++];
    cmp #8
    bne not8
    lda (pointer),y
    sta pause
    inc pointer
    bne pntr_ok2
    inc pointer+1
pntr_ok2:
    jmp loop
    
not8:
    cmp #9
    bne not9
    lda #<_default_msg
    sta pointer
    sta _scroll_msg
    lda #>_default_msg
    sta pointer+1
    sta _scroll_msg+1
    jmp loop    
    
not9:
;        while(new_char < 32);
    and #$E0
    beq loop

;       output new char
    lda new_char
    sta _scroll_char
    jmp _scroll_left

.endproc
