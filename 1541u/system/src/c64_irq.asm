	.setcpu		"6502"
	.smart		on
	.autoimport	on
    .include   "mapper.inc"
    .include   "gpio.inc"

    .export     _cia1_imsk
    .export     _cia1_test
;    .export     _cia2_imsk    
    .export     _init_cia_handler
    .export     _handle_cia_irq
    .export     _restore_cia
    
.segment	"FASTCODE"

_cia1_imsk:   .res  1,$00
_cia1_test:   .res  1,$00
;_cia2_imsk:   .res  1,$00


.proc _handle_cia_irq

    lda #'C'
    sta UART_DATA  ; temporary to check the correct handling of cia interrupts

    lda CIA1_ICR
    and #$7F
    and _cia1_test
    ora _cia1_imsk
    sta _cia1_imsk
    sta CIA1_ICR   ; turn further interrupts of this kind off

    asl _cia1_test ; next time test next bit
    
;    lda CIA2_ICR
;    and #$7F
;    ora _cia2_imsk
;    sta _cia2_imsk
;    sta CIA2_ICR   ; turn further interrupts off

    RTS

.endproc

.proc _init_cia_handler
    lda #0
    sta _cia1_imsk
;    sta _cia2_imsk
    lda CIA1_ICR
;    lda CIA2_ICR
    lda #1
    sta _cia1_test
    rts

.endproc

.proc _restore_cia
    lda _cia1_imsk
    ora #$80
    sta CIA1_ICR
;    lda _cia2_imsk
;    ora #$80
;    sta CIA2_ICR
    rts    

.endproc

