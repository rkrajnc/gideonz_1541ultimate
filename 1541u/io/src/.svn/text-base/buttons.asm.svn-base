;-------------------------------------------------------------------------------
;--
;--  (C) COPYRIGHT 2008, Gideon's Logic Architectures
;--
;-------------------------------------------------------------------------------
;-- Project    : Ultimate 1541
;-- Title      : buttons.asm
;-------------------------------------------------------------------------------
;-- Abstract:
;--   Button handling for stand alone mode.
;--   This module reads the button inputs, debounces and separates single
;--   pushes from the holding the buttons longer.
;-------------------------------------------------------------------------------

	.setcpu		"6502"
    
    .include   "gpio.inc"
    .autoimport
    
    .segment    "DATA"
btn_tmp:        .res 1
btn_cur:        .byte 0
btn_prv:        .byte 0
btn_hold:       .byte 0
hold_cnt:       .res 3
mask:           .byte 0

_button_curr:   .byte 0
_button_hold:   .byte 0
_button_event:  .byte 0

    .segment    "FASTCODE"

_read_buttons:  ; this routine is called 20 times per second

    ; debounce
    lda GPIO_IN
    eor #BUTTONS
    and #BUTTONS
    sta btn_tmp
    lda GPIO_IN
    eor #BUTTONS
    and #BUTTONS
    cmp btn_tmp
    bne _read_buttons        
    sta btn_cur
    sta _button_curr
    
    ; detect falling edge
    eor #BUTTONS
    and btn_prv
    eor #$FF
    ora btn_hold ; and not btn_hold
    eor #$FF
    sta _button_event
            
    lda #1
    sta mask
    ldx #0
hold_loop:
    lda btn_cur
    and mask
    bne btn_on
    lda #0
    sta hold_cnt,x
    lda btn_hold ; going to clear corresponding bit in hold state
    eor #$FF
    ora mask
    eor #$FF
    sta btn_hold
    jmp next_btn
btn_on:
    lda btn_hold
    and mask
    bne next_btn ; already set
count:
    lda hold_cnt,x
    cmp #30
    beq hold_det
    inc hold_cnt,x
    jmp next_btn
hold_det:
    ; update state
    lda btn_hold
    ora mask
    sta btn_hold
    ; update event
    lda _button_hold
    ora mask
    sta _button_hold
next_btn:
    asl mask
    inx
    cpx #3
    bne hold_loop

    lda btn_cur
    sta btn_prv
    rts

    .export  _read_buttons
    .export  _button_event
    .export  _button_hold
    .export  _button_curr
    .export  _clear_button_event
    
_clear_button_event:
    sei
    jsr popa
    eor #$FF
    and _button_hold
    sta _button_hold
    jsr popa
    eor #$FF
    and _button_event
    sta _button_event
    cli
    rts
    