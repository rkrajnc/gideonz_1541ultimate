	.setcpu		"6502"
	.smart		on
	.autoimport	on
    .export     _clear_screen
    .export     _scroll_down
    .export     _scroll_up
    .export     _scroll_left
	.export		_char_out
	
    .export     _start_pos
    .export     _num_lines
    .export     _width
    .export		_do_color
	.export		_clear_line
	.export     _cursor_x
	.export		_cursor_y
	.export		_cursor_pos
	.include    "mapper.inc"
	
.segment "ZEROPAGE"
src_ptr:		.res 2,0
dest_ptr:		.res 2,0	
scroll_ptr:     .res 2,0
chartmp:	    .byte 0
_cursor_x:		.byte 0
_cursor_y:		.byte 0
_cursor_pos:	.word 0

.segment "FASTCODE"

bu_map1:        .res  2,$00
bu_map2:        .res  2,$00
_start_pos:		.word 0
_num_lines:		.byte 25
_width:			.byte 40
_do_color:		.byte 1
_clear_line:	.byte 1

.segment "LIBCODE"

linedown:
	; one line down src
	lda src_ptr
	clc
	adc #40
	sta src_ptr
	lda src_ptr+1
	adc #0
	sta src_ptr+1

	; one line down dest
	lda dest_ptr
	clc
	adc #40
	sta dest_ptr
	lda dest_ptr+1
	adc #0
	sta dest_ptr+1
	rts

lineup:
	; one line up src
	lda src_ptr
	sec
	sbc #40
	sta src_ptr
	lda src_ptr+1
	sbc #0
	sta src_ptr+1

	; one line up dest
	lda dest_ptr
	sec
	sbc #40
	sta dest_ptr
	lda dest_ptr+1
	sbc #0
	sta dest_ptr+1
	rts


.segment "LIBCODE"
.proc _scroll_down  ; moves the data on the screen upward

	; Use MAP1
	lda #<MAP_SCR
	sta MAPPER_MAP1L
	lda #>MAP_SCR
	sta MAPPER_MAP1H 
	lda #<SCREEN1
	clc
	adc	_start_pos
	sta dest_ptr
	lda #>SCREEN1
	adc _start_pos+1
	sta dest_ptr+1

	; destination is one line higher, or in other words: source is one lower
	lda dest_ptr
	clc
	adc #40
	sta src_ptr
	lda dest_ptr+1
	adc #0
	sta src_ptr+1

	jsr do_scroll_down

	lda _clear_line
	beq no_clear

	ldy #0
	lda #32
clearloop:
	sta (dest_ptr),y
	iny
	cpy _width
	bne	clearloop

no_clear:
	lda _do_color
	bne color_down
	rts

color_down:	
	lda #<COLOR3
	clc
	adc	_start_pos
	sta dest_ptr
	lda #>COLOR3
	adc _start_pos+1
	sta dest_ptr+1

	lda dest_ptr
	clc
	adc #40
	sta src_ptr
	lda dest_ptr+1
	adc #0
	sta src_ptr+1

	jmp do_scroll_down	
	
.segment "FASTCODE"

do_scroll_down:
	ldx _num_lines
lineloop:
	dex
	beq done
	ldy #0
charloop:
	lda (src_ptr),y
	sta (dest_ptr),y
	iny
	cpy _width
	bne	charloop

	jsr linedown

	jmp lineloop
done:
	rts
	
.endproc

.segment "LIBCODE"
.proc _scroll_up  ; moves the data on the screen down

	; Use MAP1
	lda #<MAP_SCR
	sta MAPPER_MAP1L
	lda #>MAP_SCR
	sta MAPPER_MAP1H
	
	lda #<SCREEN1
	clc
	adc	_start_pos
	sta src_ptr
	lda #>SCREEN1
	adc _start_pos+1
	sta src_ptr+1

	; destination is one line lower
	lda src_ptr
	clc
	adc #40
	sta dest_ptr
	lda src_ptr+1
	adc #0
	sta dest_ptr+1

	ldy _num_lines
	dey
	beq ready
godown:
	dey
	beq under
	jsr linedown
	jmp godown
ready:
	rts

under:
	jsr do_scroll_up

	lda _clear_line
	beq no_clear

	ldy #0
	lda #32 ; space
clearloop:
	sta (dest_ptr),y
	iny
	cpy _width
	bne	clearloop

no_clear:
	lda _do_color
	beq ready
		
	lda #<COLOR3
	clc
	adc	_start_pos
	sta src_ptr
	lda #>COLOR3
	adc _start_pos+1
	sta src_ptr+1

	; destination is one line lower
	lda src_ptr
	clc
	adc #40
	sta dest_ptr
	lda src_ptr+1
	adc #0
	sta dest_ptr+1

	jmp do_scroll_up	
	
.segment "FASTCODE"

do_scroll_up:
	ldx _num_lines
lineloop:
	dex
	beq done
	ldy #0
charloop:
	lda (src_ptr),y
	sta (dest_ptr),y
	iny
	cpy _width
	bne	charloop

	jsr lineup
	jmp lineloop
done:
	rts

.endproc

.segment "LIBCODE"
.proc _char_out
	tya
	pha

    lda MAPPER_MAP1L
    sta bu_map1
    lda MAPPER_MAP1H
    sta bu_map1+1
    lda #<MAP_SCR
    sta MAPPER_MAP1L
    lda #>MAP_SCR
    sta MAPPER_MAP1H

	jsr popa
	sta chartmp
	
	cmp #13 ; carriage return
	bne not13
	jsr cursor_down
	jsr cursor_home
	
	jmp done
	
not13:
    cmp #10 ; line feed (handle same as carriage return for now)
    bne not10
    
	jsr cursor_down
	jsr cursor_home
	
	jmp done

not10:
	; check for other control chars here #TODO#
	and #$E0
	beq done ; ignore other control chars

not_ctrl:
	; put character and move to next position
	ldy #0
    lda chartmp
	sta (_cursor_pos),y
	
	; move right
	inc _cursor_pos
	bne nowrap
	inc _cursor_pos+1
nowrap:
	inc _cursor_x
	ldx _cursor_x
	cpx _width
	bne xok

	jsr cursor_down
	jsr cursor_home
xok:


done:
    lda bu_map1
    sta MAPPER_MAP1L
    lda bu_map1+1
    sta MAPPER_MAP1H
        
	pla
	tay
	rts

cursor_home:
	; move to beginning of line
	lda _cursor_pos
	sec
	sbc _cursor_x
	sta _cursor_pos
	lda _cursor_pos+1
	sbc #0
	sta _cursor_pos+1
	lda #0
	sta _cursor_x
	rts

cursor_down:
	; check if we are on the last line
	ldx _cursor_y
	inx
	cpx _num_lines
	bne yok
	; we need to scroll and keep the current cursor position
	jsr _scroll_down
	rts
yok:
	stx _cursor_y
	lda _cursor_pos
	clc
	adc #40
	sta _cursor_pos
	lda _cursor_pos+1
	adc #0
	sta _cursor_pos+1
	rts

.endproc

.segment "LIBCODE"
.proc _clear_screen  ; place spaces

    .export     _set_color

    tya
    pha
        
    jsr popa
    sta chartmp

	; Use MAP1
	lda #<MAP_SCR
	sta MAPPER_MAP1L
	lda #>MAP_SCR
	sta MAPPER_MAP1H
	
	lda #<SCREEN1
	clc
	adc	_start_pos
	sta dest_ptr
	lda #>SCREEN1
	adc _start_pos+1
	sta dest_ptr+1

	ldx _num_lines
lineloop:
	lda #32
	ldy #0
charloop:
	sta (dest_ptr),y
	iny
	cpy _width
	bne	charloop

	jsr linedown

	dex
	bne lineloop

    bit chartmp
    bmi no_color
    
clr_color:
	; Use MAP1
	lda #<COLOR3
	clc
	adc	_start_pos
	sta dest_ptr
	lda #>COLOR3
	adc _start_pos+1
	sta dest_ptr+1

	ldx _num_lines
lineloop2:
    lda chartmp
	ldy #0
colorloop:
	sta (dest_ptr),y
	iny
	cpy _width
	bne	colorloop

	jsr linedown

	dex
	bne lineloop2
    
no_color:
    pla
    tay
    rts

_set_color:
    tya
    pha
        
    jsr popa
    sta chartmp

    jmp clr_color

.endproc

.segment "LIBCODE"
.proc _scroll_left  ; scrolls last line of the screen one position to the left
                    ; and places new character

    lda MAPPER_MAP2L
    sta bu_map2
    lda MAPPER_MAP2H
    sta bu_map2+1
    lda #<MAP_SCR
    sta MAPPER_MAP2L
    lda #>MAP_SCR
    sta MAPPER_MAP2H

    lda #<(SCREEN2 + $03c0)
    sta scroll_ptr
    lda #>(SCREEN2 + $03c0)
    sta scroll_ptr+1
    
    ldy #1
sloop:
    lda (scroll_ptr),y
    dey
    sta (scroll_ptr),y
    iny
    iny
    cpy #40
    bne sloop
    
    lda _scroll_char
    dey
    sta (scroll_ptr),y

    lda bu_map2
    sta MAPPER_MAP2L
    lda bu_map2+1
    sta MAPPER_MAP2H

    rts

    .segment    "BSS"
_scroll_char:    .res 1

    .export     _scroll_char

.endproc
