	.setcpu		"6502"
	.smart		on
;	.autoimport	on
    .export     _scan_keyboard
    .export     _kb_buf_getch
    .export     _kb_buf
    .export     _kb_buf_size
    .export     _kb_buf_cnt
    .export     _wait_kb_free
    
    .include   "mapper.inc"
    .include   "gpio.inc"
	
.segment	"ZEROPAGE"
kb_map:		.res 2, $00
last_mtrx:  .res 1, $00 ; CB
last_mtrx2: .res 1, $00 ; C5

.segment    "FASTDATA"  

do_repeat:    .byte $00 ; 028A
rep_speed:    .byte $03 ; 028B
first_dly:    .byte $10 ; 028C
shift_flag:   .byte $00 ; 028D
shift_bck:	  .byte $00 ; 028E

_kb_buf_cnt:  .byte $00 ; C6
_kb_buf_size: .byte $0A ; 0289
_kb_buf:      .res 10,$00 ; buffer

.segment	"LIBCODE"

.proc	_scan_keyboard

.segment	"RODATA"
kb_maps:    .word kb_map1, kb_map2, kb_map1, kb_map1

; Standard map
kb_map1:    .byte $14,$0D,$1D,$88,$85,$86,$87,$11
            .byte $33,$77,$61,$34,$7A,$73,$65,$01
            .byte $35,$72,$64,$36,$63,$66,$74,$78
            .byte $37,$79,$67,$38,$62,$68,$75,$76
            .byte $39,$69,$6A,$30,$6D,$6B,$6F,$6E
            .byte $2B,$70,$6C,$2D,$2E,$3A,$40,$2C
            .byte $5C,$2A,$3B,$13,$01,$3D,$7C,$2F
            .byte $31,$60,$04,$32,$20,$02,$71,$03
            .byte $FF

; Shifted map:
kb_map2:    .byte $94,$8D,$9D,$8C,$89,$8A,$8B,$91
            .byte $23,$57,$41,$24,$5A,$53,$45,$01
            .byte $25,$52,$44,$26,$43,$46,$54,$58
            .byte $27,$59,$47,$28,$42,$48,$55,$56
            .byte $29,$49,$4A,$30,$4D,$4B,$4F,$4E
            .byte $7B,$50,$4C,$7D,$3E,$5B,$BA,$3C
            .byte $5F,$60,$5D,$93,$01,$3D,$5E,$3F
            .byte $21,$7E,$04,$22,$A0,$02,$51,$83
            .byte $FF


.segment	"LIBCODE"
LEA87:
    LDA #$00
    STA shift_flag  ; Flag: Shift Keys
    LDY #$40
    STY last_mtrx ; Matrix value of last Key pressed
    STA CIA1_DPA  ; CIA1: Data Port A (Keyboard, Joystick, Paddles)
    LDX CIA1_DPB  ; CIA1: Data Port B (Keyboard, Joystick, Paddles)
    CPX #$FF
    BEQ LEAFB     ; Process Key Image
    TAY
    LDA #<kb_map1
    STA kb_map    ; Vector: Current Keyboard decoding Table
    LDA #>kb_map1
    STA kb_map+1  ; Vector: Current Keyboard decoding Table
    LDA #$FE
    STA CIA1_DPA  ; CIA1: Data Port A (Keyboard, Joystick, Paddles)

LEAA8:
    LDX #$08
    PHA

LEAAB:
    LDA CIA1_DPB  ; CIA1: Data Port B (Keyboard, Joystick, Paddles)
    CMP CIA1_DPB  ; CIA1: Data Port B (Keyboard, Joystick, Paddles)
    BNE LEAAB

LEAB3:
    LSR
    BCS LEACC
    PHA
    LDA (kb_map),Y   ; Vector: Current Keyboard decoding Table
    CMP #$05
    BCS LEAC9
    CMP #$03
    BEQ LEAC9
    ORA shift_flag    ; Flag: Shift Keys
    STA shift_flag    ; Flag: Shift Keys
    BPL LEACB

LEAC9:
    STY last_mtrx     ; Matrix value of last Key pressed

LEACB:
    PLA

LEACC:
    INY
    CPY #$41
    BCS LEADC
    DEX
    BNE LEAB3
    SEC
    PLA
    ROL
    STA CIA1_DPA   ; CIA1: Data Port A (Keyboard, Joystick, Paddles)
    BNE LEAA8

LEADC:
    PLA
    JMP LEB48   ; Vector: Routine to determine Keyboard table

LEAE0:
    LDY last_mtrx     ; Matrix value of last Key pressed
    LDA (kb_map),Y   ; Vector: Current Keyboard decoding Table
    TAX
    CPY last_mtrx2    ; Matrix value of last Key pressed
    BEQ LEAF0
    LDY #$10
    STY first_dly     ; Repeat Key: First repeat delay Counter
    BNE LEB26

LEAF0:
    AND #$7F
    BIT do_repeat     ; Flag: Repeat keys
    BMI LEB0D
    BVS LEB42
    CMP #$7F

LEAFB:
    BEQ LEB26
    CMP #$14
    BEQ LEB0D
    CMP #$20
    BEQ LEB0D
    CMP #$1D
    BEQ LEB0D
    CMP #$11
    BNE LEB42

LEB0D:
    LDY first_dly ; Repeat Key: First repeat delay Counter
    BEQ LEB17
    DEC first_dly ; Repeat Key: First repeat delay Counter
    BNE LEB42

LEB17:
    DEC rep_speed ; Repeat Key: Speed Counter
    BNE LEB42
    LDY #$04
    STY rep_speed   ; Repeat Key: Speed Counter
    LDY _kb_buf_cnt ; Number of Characters in Keyboard Buffer queue
    DEY
    BPL LEB42

LEB26:
    LDY last_mtrx    ; Matrix value of last Key pressed
    STY last_mtrx2   ; Matrix value of last Key pressed
    LDY shift_flag   ; Flag: Shift Keys
    STY shift_bck    ; Last Shift Key used for debouncing
    CPX #$FF
    BEQ LEB42
    TXA
    LDX _kb_buf_cnt  ; Number of Characters in Keyboard Buffer queue
    CPX _kb_buf_size ; Maximum number of Bytes in Keyboard Buffer
    BCS LEB42
    STA _kb_buf,X    ; Keyboard Buffer Queue (FIFO)
    INX
    STX _kb_buf_cnt  ; Number of Characters in Keyboard Buffer queue

LEB42:
    LDA #$7F
    STA CIA1_DPA    ; CIA1: Data Port A (Keyboard, Joystick, Paddles)

    lda #1
    sta VIC_IREG

    RTS

; Check for Shift, CTRL, C=

LEB48:
    LDA shift_flag ; Flag: Shift Keys
    CMP #$03
    BNE LEB64
    CMP shift_bck ; Last Shift Key used for debouncing
    BEQ LEB42     ; Process Key Image
    JMP LEB76

LEB64:
    ASL
    CMP #$08
    BCC LEB6B
    LDA #$06

LEB6B:
    TAX
    LDA kb_maps,X    ; Pointers to Keyboard decoding tables
    STA kb_map       ; Vector: Current Keyboard decoding Table
    LDA kb_maps+1,X  ; Pointers to Keyboard decoding tables
    STA kb_map+1     ; Vector: Current Keyboard decoding Table

LEB76:
    JMP LEAE0     ; Process Key Image

.endproc

.proc _wait_kb_free
    sei
    lda #$00      ; select ALL rows
    sta CIA1_DPA  ; CIA1: Data Port A (Keyboard, Joystick, Paddles)
readkb:
    lda CIA1_DPB  ; CIA1: Data Port B (Keyboard, Joystick, Paddles)
    cmp CIA1_DPB  ; CIA1: Data Port B (Keyboard, Joystick, Paddles)
    bne readkb
    cmp #$FF      ; all keys released?
    bne readkb
    lda #$FF
    sta CIA1_DPA
    cli
    rts
.endproc

.proc _kb_buf_getch

    sei
    lda _kb_buf_cnt
    beq done

    ldy _kb_buf
    ldx #0
loop:
    lda _kb_buf+1,x
    sta _kb_buf,x
    inx
    cpx _kb_buf_cnt
    bne loop
    
    dec _kb_buf_cnt
    tya    
done:
    ldx #0
    cli
    rts
    
.endproc

