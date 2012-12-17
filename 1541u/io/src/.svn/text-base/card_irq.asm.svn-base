;-------------------------------------------------------------------------------
;--
;--  (C) COPYRIGHT 2008, Gideon's Logic Architectures
;--
;-------------------------------------------------------------------------------
;-- Project    : Ultimate 1541
;-- Title      : card_irq.asm
;-------------------------------------------------------------------------------
;-- Abstract:
;--   SD-card interrupt for change detection
;--
;-------------------------------------------------------------------------------

	.setcpu		"6502"
    
    .include   "gpio.inc"

    .segment    "DATA"
_card_det:      .byte 0
_card_change:   .byte 0
card_det_cnt:   .byte 0

    .segment    "FASTCODE"
_check_sd:
    lda GPIO_IN2
    and #SD_CARDDET
    beq card_seen
no_card:
    lda _card_det
    beq no_change
    inc card_det_cnt
    lda card_det_cnt
    cmp #8
    beq card_removed
    rts
card_removed:
    lda #0
    sta _card_det
    lda #1
    sta _card_change
    rts
card_seen:
    lda _card_det
    bne no_change
    lda #0
    sta card_det_cnt
    lda #1
    sta _card_det
    sta _card_change
no_change:
    rts

    .export  _card_det
    .export  _card_change
    .export  _check_sd
