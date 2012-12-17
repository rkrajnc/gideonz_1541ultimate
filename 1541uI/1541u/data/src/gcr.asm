;-------------------------------------------------------------------------------
;--
;--  (C) COPYRIGHT 2007, Gideon's Logic Architectures
;--
;-------------------------------------------------------------------------------
;-- Project    : Ultimate 1541
;-- Title      : gcr.asm
;-------------------------------------------------------------------------------
;-- Abstract:
;	Converting D64 images to GCR and loading it to the emulated 1541's buffer
;--
;-------------------------------------------------------------------------------

	.fopt		compiler,"cc65 v 2.11.0"
	.setcpu		"6502"
	.smart		on
	.autoimport	on
	.importzp	sp,sreg,tmp1
	.import		_f_open
	.import		_f_read
	.import		_f_lseek
	.import     _d64_load_params
    .export     _find_trackstart
	.export		_load_d64
	.export		bin2gcr
	.export     gcr2bin
    .export     _get_sector
    .export     _get_num_sectors
    .export     _ts2sectnr
    .export     _sector_buf
    .export     _header
    .include    "mapper.inc"
    .include    "gpio.inc"

.segment	"FASTCODE"

_CalcBlockChecksum:
    lda #$00
    ldy #0
calcloop:
    eor (tmp1),y
    iny
    bne calcloop
    rts

.segment    "BSS"
_header:     .res 8,$00

.segment	"ZEROPAGE"
bin_in:		.res 4, $00
bin_out:    .res 4, $00
gcr_buf:	.res 5, $00
work_zp:	.res 8, $00

.segment    "ZEROPAGE"
in_pnt:     .res    2,$00
out_pnt:    .res    2,$00

.segment    "FASTDATA"
_sector_buf: .res	260,$00
end_addr:   .byte  $00, $60
file_pnt:   .res   2, $00
sec_start:  .res   2, $00

.segment	"FASTDATA"

region_end:
    .byte   17,24,30,35,52,59,65,255
region_secs:
    .byte   21,19,18,17,21,19,18,17
region_gap:
    .byte   9,19,13,10,9,19,13,10
region_len:
    .word   $1E00, $1BE0, $1A00, $1860, $1E00, $1BE0, $1A00, $1860
last_region:
    .byte   3

.segment	"FASTCODE"
.proc	bin2gcr

.segment	"FASTDATA"
gcrtable:
	.byte	$0A,$0B,$12,$13,$0E,$0F,$16,$17
	.byte	$09,$19,$1A,$1B,$0D,$1D,$1E,$15

.segment	"FASTCODE"
	ldy #0
loop1:
	lda bin_in,y
	lsr
	lsr
	lsr
	lsr
	tax
	lda gcrtable,x
	sta work_zp,y
	lda bin_in,y
	and #$0F
	tax
	lda gcrtable,x
	sta work_zp+4,y
	iny
	cpy #4
	bne loop1

	; all GCR codes are now in work_zp (Order: 0 4 1 5 2 6 3 7)
	
	; now, grasp all 5-bit words together in 5 bytes	
	lda #0
	sta gcr_buf+1
	sta gcr_buf+2
	sta gcr_buf+3
	sta gcr_buf+4

	; 76543210 => 76543210
	;    xxxxx    xxxxx
	lda work_zp
	asl
	asl
	asl
	sta gcr_buf
    
	; 76543210 => 76543210 76543210
	;    xxxxx         xxx xx
	lda work_zp+4
	lsr
	ror gcr_buf+1
	lsr
	ror gcr_buf+1
	ora gcr_buf
	sta gcr_buf
			
	; 76543210 => 76543210 76543210
	;    xxxxx               xxxxx
	lda work_zp+1
	asl
	ora gcr_buf+1
	sta gcr_buf+1
    	
	; 76543210 => 76543210 76543210 76543210
	;    xxxxx                    x xxxx
	lda work_zp+5
	lsr
	ror gcr_buf+2
	lsr
	ror gcr_buf+2
	lsr
	ror gcr_buf+2
	lsr
	ror gcr_buf+2
	ora gcr_buf+1
	sta gcr_buf+1
		
	; 76543210 => 76543210 76543210 76543210 76543210
	;    xxxxx                          xxxx x
	lda work_zp+2
	lsr
	ror gcr_buf+3
	ora gcr_buf+2
	sta gcr_buf+2

	; 76543210 => 76543210 76543210 76543210 76543210
	;    xxxxx                                xxxxx
	lda work_zp+6
	asl
	asl
	ora gcr_buf+3
	sta gcr_buf+3

	; 76543210 => 76543210 76543210 76543210 76543210 76543210
	;    xxxxx                                     xx xxx
	lda work_zp+3
	lsr
	ror gcr_buf+4
	lsr
	ror gcr_buf+4
	lsr
	ror gcr_buf+4
	ora gcr_buf+3
	sta gcr_buf+3
	
	; 76543210 => 76543210 76543210 76543210 76543210 76543210
	;    xxxxx                                           xxxxx
		
	lda work_zp+7
	ora gcr_buf+4
	sta gcr_buf+4

	rts

.endproc
	
.segment	"FASTCODE"
.proc	gcr2bin

.segment	"FASTDATA"
bintable:
    .byte   $AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA ; 00
    .byte   $AA,$08,$00,$01,$AA,$0C,$04,$05 ; 08
    .byte   $AA,$AA,$02,$03,$AA,$0F,$06,$07 ; 10
    .byte   $AA,$09,$0A,$0B,$AA,$0D,$0E,$AA ; 18

.segment	"FASTCODE"

	; 76543210 => 76543210 (0)
	; xxxxx          xxxxx
    lda gcr_buf
    lsr
    lsr
    lsr
    sta work_zp
    
	; 76543210 76543210 => 76543210 (4)
	;      xxx xx             xxxxx    
			
    lda gcr_buf
    sta work_zp+4
    lda gcr_buf+1
    asl
    rol work_zp+4
    asl
    rol work_zp+4
    
	; 76543210 76543210 => 76543210 (1)
	;            xxxxx        xxxxx    
    	
    lda gcr_buf+1
    lsr
    sta work_zp+1

	; 76543210 76543210 76543210 => 76543210 (5)
	;                 x xxxx           xxxxx    
		
    lda gcr_buf+2
    sta work_zp+5
    lda gcr_buf+1
    lsr
    ror work_zp+5
    lda work_zp+5
    lsr
    lsr
    lsr
    sta work_zp+5

	; 76543210 76543210 76543210 76543210 => 76543210 (2)
	;                       xxxx x              xxxxx    

    lda gcr_buf+2
    sta work_zp+2
    lda gcr_buf+3
    asl
    rol work_zp+2

	; 76543210 76543210 76543210 76543210 => 76543210 (6)
	;                             xxxxx         xxxxx    

    lda gcr_buf+3
    lsr
    lsr
    sta work_zp+6

	; 76543210 76543210 76543210 76543210 76543210 => 76543210 (3)
	;                                  xx xxx            xxxxx    

    lda gcr_buf+3
    sta work_zp+3
    lda gcr_buf+4
    asl
    rol work_zp+3
    asl
    rol work_zp+3
    asl
    rol work_zp+3
    
	; 76543210 76543210 76543210 76543210 76543210 => 76543210 (7)
	;                                        xxxxx       xxxxx    

    lda gcr_buf+4
    sta work_zp+7

	; all GCR codes are now in work_zp (Order: 0 4 1 5 2 6 3 7), unmasked

    ldy #0
loop1:
    lda work_zp,y
    and #$1F
    tax
    lda bintable,x
    asl
    asl
    asl
    asl
    sta bin_out,y
    lda work_zp+4,y
    and #$1F
    tax
    lda bintable,x
    ora bin_out,y
    sta bin_out,y
    iny
    cpy #4
    bne loop1
    
    rts

.endproc

.segment	"FASTCODE"
.proc	_find_trackstart

.segment    "FASTDATA"
match_ptrn: .byte   $FF, $52, $40, $05, $28
match_mask: .byte   $FF, $FF, $C0, $0F, $FC

.segment	"FASTCODE"

    ; find pattern
    lda #<MAP1_ADDR
    sta in_pnt
    lda #>MAP1_ADDR
    sta in_pnt+1
    ldx #$1F
try_next:
    ldy #0
match_loop:
    lda (in_pnt),y
    and match_mask,y
    cmp match_ptrn,y
    bne no_match
    iny
    cpy #5
    bne match_loop
    ; return pointer    
    lda in_pnt
    ldx in_pnt+1
    rts
no_match:
    inc in_pnt
    bne try_next
    inc in_pnt+1
    dex
    bne try_next
    lda #0
    tax
    rts
    
.endproc

.segment	"FASTCODE"

.proc _get_num_sectors
    jsr popa    ;; mode
    cmp #0
    beq d64
    cmp #1
    beq d71
    cmp #2
    beq d81
    jsr popa    ;; eat other stack entry
    lda #$ff
    tax
    rts
d71:lda #7
    sta last_region
    jmp doit
d64:lda #3
    sta last_region
doit:
    jsr popa    ;; track
    jsr get_track_len
    ldx #0
    rts
d81:
    jsr popa    ;; eat track parameter
    lda #40
    ldx #0
    rts

.endproc

.proc  get_track_len
    ldy #0
next_region:
    cmp region_end,y
    bmi region_found
    iny
    cpy last_region
    bne next_region
region_found:
    lda region_secs,y
    rts

.endproc

.proc   _ts2sectnr
    jsr popa ;; mode
    cmp #0
    beq d64
    cmp #1
    beq d71
    cmp #2
    beq d81        
    jsr popax   ;; eat other stack entries
    lda #$ff
    tax
    rts
d71:lda #7
    sta last_region
    jmp doit    
d64:lda #3
    sta last_region
doit:
    jsr popa
    tax

    lda #0
    sta tmp1
    sta tmp1+1

loop:
    dex
    beq no_trk
    dex
    txa  ; a = x - 1
    inx
    jsr get_track_len
    clc
    adc tmp1
    sta tmp1
    lda tmp1+1
    adc #0
    sta tmp1+1
    jmp loop
no_trk:
    jsr popa
    clc
    adc tmp1
    sta tmp1
    lda tmp1+1
    adc #0
    sta tmp1+1

    tax
    lda tmp1
    rts

d81:    ;; just do (track-1)*40 + sector
    lda #0
    sta tmp1+1
    jsr popa
    tax
    dex
    txa
    asl
    asl
    sta tmp1
    rol tmp1+1 ;  tmp = (track-1)*4
    lda tmp1
    ldx tmp1+1
    jsr mulax10
    sta tmp1
    stx tmp1+1
    jmp no_trk ;; add sector to it

.endproc

.proc	_get_sector

    jsr popa
    ldy #0
next_region:
    cmp region_end,y
    bmi region_found
    iny
    cpy #3
    bne next_region
region_found:

    tya
    asl
    tay
    lda region_len,y
    sta end_addr
    lda region_len+1,y
    clc
    adc #>MAP1_ADDR
    sta end_addr+1

    lda #<_sector_buf
    sta out_pnt
    lda #>_sector_buf
    sta out_pnt+1
    
    ; assume in_pnt to be pointing at the beginning of header (FF 52 ...)
skip_sync1:
    jsr get_byte
    cmp #$FF
    beq skip_sync1

    sta gcr_buf
    ldx #1
hdr1_loop:
    jsr get_byte
    sta gcr_buf,x
    inx
    cpx #5
    bne hdr1_loop
    jsr gcr2bin
    ldx #0
hdr1_copy:
    lda bin_out,x
    sta _header,x
    inx
    cpx #4
    bne hdr1_copy

    ldx #0
hdr2_loop:
    jsr get_byte
    sta gcr_buf,x
    inx
    cpx #5
    bne hdr2_loop
    jsr gcr2bin
    ldx #0
hdr2_copy:
    lda bin_out,x
    sta _header+4,x
    inx
    cpx #4
    bne hdr2_copy
    
skip_gap:
    jsr get_byte
    cmp #$FF
    bne skip_gap

    jsr get_byte
    jsr get_byte
    jsr get_byte

skip_sync2:
    jsr get_byte
    cmp #$FF
    beq skip_sync2

    dec in_pnt
    lda in_pnt
    cmp #$FF
    bne page_ok
    dec in_pnt+1

page_ok:
    lda in_pnt
    sta sec_start
    lda in_pnt+1
    sta sec_start+1

    ldy #65
loop1:
    tya
    pha
    ldx #0
loop2:
    jsr get_byte
    sta gcr_buf,x
    inx
    cpx #5
    bne loop2
    jsr gcr2bin
    ldy #0
loop3:
    lda bin_out,y
    sta (out_pnt),y
    iny
    cpy #4
    bne loop3
    lda out_pnt
    clc
    adc #4
    sta out_pnt
    lda out_pnt+1
    adc #0
    sta out_pnt+1
    
    pla
    tay
    dey
    bne loop1

skip_gap2:
    jsr get_byte
    cmp #$FF
    bne skip_gap2
    
klaar:
;    lda #<_header
;    ldx #>_header
    lda sec_start
    ldx sec_start+1
    rts

get_byte:
    ldy #0
    lda (in_pnt),y
    pha
    inc in_pnt
    bne nohi
    inc in_pnt+1
nohi:
    lda in_pnt+1
    cmp end_addr+1
    bne ok
    lda in_pnt
    cmp end_addr
    bne ok
    lda #<MAP1_ADDR
    sta in_pnt
    lda #>MAP1_ADDR
    sta in_pnt+1
ok:
    pla
    rts

.endproc

; ---------------------------------------------------------------
; void __near__ load_d64 (BYTE*)
; ---------------------------------------------------------------

.segment	"FASTCODE"

.proc	_load_d64

.segment	"FASTDATA"

header_buf: .byte   $08,0,0,0,0,0,$0F,$0F ; 8, chk, sec, track, id1, id0, 0f, 0f
;                   01010 01001 ????? ????? 01010 01010 ????? ?????                    
;                   01010010 01?????? ????0101 001010?? ????????                    
;                   52       &C0=40   &0F=05   &FC=28                      

.segment	"FASTDATA"
id:         .res    2,$00
bytes_read: .res    2,$00
track:      .res    1,$00
sector:     .res    1,$00
sectors:    .res    1,$00
sector_gap: .res    1,$00
swap_delay: .res    1,$00

.segment	"FASTCODE"

    jsr     _d64_load_params
    
    ;; store swap delay
    jsr     popa
    sta     swap_delay

    ;; store write protect flag
    jsr     popa
    pha

    ;; store file pointer parameter
    jsr     popax
    sta     file_pnt
    stx     file_pnt+1

    ; open door
    lda     #FLOPPY_INS
    sta     GPIO_CLEAR2
;
; f_lseek(&mountfile, id_offset);
;
    ; push file position $0165A2 onto the stack (position of ID)
	lda     file_pnt
	ldx     file_pnt+1
    jsr     pushax
    
	ldx     #$65
	lda     #$01
	sta     sreg
	lda     #$00
	sta     sreg+1
	lda     #$A2
	jsr     pusheax

	jsr     _f_lseek
;
; fres = f_read(&mountfile, &id, 2, &bytes_read);
;
	lda     file_pnt
	ldx     file_pnt+1
    jsr     pushax
    lda     #<(id)
    ldx     #>(id)
    jsr     pushax
    lda     #2
    ldx     #0
    jsr     pushax
    lda     #<(bytes_read)
    ldx     #>(bytes_read)
    jsr     pushax
    jsr     _f_read    

;
; f_lseek(&mountfile, 0L);
;
	lda     file_pnt
	ldx     file_pnt+1
    jsr     pushax
    lda     #0
    tax
    sta     sreg
    sta     sreg+1
    jsr     pusheax
	jsr     _f_lseek

;
; for (track=0; track < 35; track++) {
;
    ldx     #0
    stx     track
    
next_track:

    lda     track
    cmp     #5
    bne     not5
    ;; on track 5 close sensor (removing disk...)
    lda     #WRITE_PROT
    sta     GPIO_SET
    jmp     do_track

not5:    
    ;; on track 30 open sensor (disk has been totally removed)
    lda     track
    cmp     #30
    bne     do_track
    lda     #WRITE_PROT
    sta     GPIO_CLEAR

do_track:
    ; debug
;   lda     track
;   clc
;   adc     #$40
;   sta     $2100 ; UART_DATA

    ldy     #0
    lda     track
next_region:
    cmp     region_end,y
    bmi     region_found
    iny
    cpy     #3
    bne     next_region
region_found:
    lda     region_secs,y
    sta     sectors
    lda     region_gap,y
    sta     sector_gap
;
; page = track; // tracks are 8k, pages are 8, too
;
    lda     track
	sta     MAPPER_MAP1L

    lda     #>MAP1_ADDR
    sta     out_pnt+1
    lda     #<MAP1_ADDR
    sta     out_pnt
    sta     MAPPER_MAP1H            
;
; for(s=0; s < secs; s++) {
;
    ldx     #0
    stx     sector

next_sector:
    ldx     sector
    stx     header_buf+2
    
    ldy     track
    iny
    sty     header_buf+3
    
    lda     id+1
    sta     header_buf+4
    
    lda     id
    sta     header_buf+5
    
;
; hdrbuf[1] = hdrbuf[2] ^ hdrbuf[3] ^ hdrbuf[4] ^ hdrbuf[5];
;
    eor     header_buf+4
    eor     header_buf+3
    eor     header_buf+2
    sta     header_buf+1
    
;
; f_read(&mountfile, &blockbuf[1], 256, &bytes_read);
;
	lda     file_pnt
	ldx     file_pnt+1
    jsr     pushax
    lda     #<(_sector_buf+1)
    ldx     #>(_sector_buf+1)
    jsr     pushax
    lda     #0
    ldx     #1
    jsr     pushax
    lda     #<(bytes_read)
    ldx     #>(bytes_read)
    jsr     pushax
    jsr     _f_read    

    lda     bytes_read+1
    cmp     #1
    beq     read_ok
    jmp     track_done

read_ok:
;
; printf(".");
;
;    lda     #$2E
;    sta     $2100 ; UART_DATA
;
; checksum = CalcBlockChecksum(&blockbuf[1]);
;
    lda     #<(_sector_buf+1)
    sta     tmp1
    lda     #>(_sector_buf+1)
    sta     tmp1+1
	jsr     _CalcBlockChecksum
	sta     _sector_buf+257
    lda     #7
    sta     _sector_buf+0
    lda     #0
    sta     _sector_buf+258
    sta     _sector_buf+259
    
    ; Put 5 sync bytes
    
    lda #$FF
    ldy #0
syncloop1:
    sta (out_pnt),y
    iny
    cpy #5
    bne syncloop1
    
    ; Put header
    ldx #0
L1: lda header_buf,x
    sta bin_in,x
    inx
    cpx #4
    bne L1
    jsr bin2gcr
    ldy #5
L2: lda gcr_buf-5,y
    sta (out_pnt),y
    iny
    cpy #10
    bne L2
    
    ldx #0
L3: lda header_buf+4,x
    sta bin_in,x
    inx
    cpx #4
    bne L3
    jsr bin2gcr
    ldy #10
L4: lda gcr_buf-10,y
    sta (out_pnt),y
    iny
    cpy #15
    bne L4
    
    ; put 9 gap bytes

    lda #$55
L5: sta (out_pnt),y
    iny
    cpy #24
    bne L5
    
    lda #$FF
L6: sta (out_pnt),y
    iny
    cpy #29
    bne L6

    lda out_pnt
    clc
    adc #29
    sta out_pnt
    lda out_pnt+1
    adc #0
    sta out_pnt+1
    
;
; for(i=0;i<65;i++) {
;
    lda #<(_sector_buf)
    sta in_pnt
    lda #>(_sector_buf)
    sta in_pnt+1
    
    ldx #0 ; i
    
L9: ldy #0
L7: lda (in_pnt),y
    sta bin_in,y
    iny
    cpy #4
    bne L7

    lda in_pnt
    clc
    adc #4
    sta in_pnt
    lda in_pnt+1
    adc #0
    sta in_pnt+1

    txa
    pha
    jsr bin2gcr
    pla
    tax
    
    ldy #0
L8: lda gcr_buf,y
    sta (out_pnt),y
    iny
    cpy #5
    bne L8

    lda out_pnt
    clc
    adc #5
    sta out_pnt
    lda out_pnt+1
    adc #0
    sta out_pnt+1

    inx
    cpx #65
    bne L9
;
; for(b=0;b<syncgap[track];b++) {
;
    lda #$55
    ldy #0
L10:
    sta (out_pnt),y
    iny
    cpy sector_gap  
    bne L10

    lda out_pnt
    clc
    adc sector_gap
    sta out_pnt
    lda out_pnt+1
    adc #0
    sta out_pnt+1    
;
; while( s < secs )
;
    inc     sector
    lda     sector
    cmp     sectors
    beq     track_done
    jmp     next_sector
    
track_done:
    lda out_pnt+1
    cmp #>MAP2_ADDR
    beq track_fill_ok
    ldy out_pnt
    lda #0
    sta out_pnt
    lda #$55
end_fill:
    sta (out_pnt),y
    iny
    bne end_fill
    inc out_pnt+1
    jmp track_done

track_fill_ok:
;
; printf("\n");
;
;    lda     #$0D
;    sta     $2100 ; UART_DATA
;    lda     #$0A
;    sta     $2100 ; UART_DATA
;
; while (track < 35; track++)
;
    lda     bytes_read+1
    cmp     #1
    bne     disk_done

    inc     track
    lda     track
    cmp     #40
    beq     disk_done
    jmp     next_track
    
disk_done:
    ;; wait with open sensor for a second
    ;; this equals to approx 1024 times a delay of 195 timer ticks  (revised)
    ldx swap_delay
    beq disk_is_in
    ldy #0
dlyloop1:
    lda #78
    sta TIMER
    lda TIMER
    bne *-3
    dey
    bne dlyloop1
    dex
    bne dlyloop1

    ;; close sensor for 0.7 seconds because of inserting new disk
    ;; this equals to approx 768 times a delay of 182 timer ticks
    lda     #WRITE_PROT
    sta     GPIO_SET

    ldx swap_delay
    ldy #0
dlyloop2:
    lda #39
    sta TIMER
    lda TIMER
    bne *-3
    dey
    bne dlyloop2
    dex
    bne dlyloop2

disk_is_in:
    pla
    bne     write_protected

    ;; insert is done.. sensor is open (in case the disk is writable)
    lda     #WRITE_PROT
    sta     GPIO_CLEAR

write_protected:
    ;; close door
    lda     #FLOPPY_INS
    sta     GPIO_SET2

    ;; now clear all dirty bits (maybe redundant?)
    lda     #0
    ldx     #0
drt:
    sta     TRACK_DIRTY,x
    inx
    cpx     #$40
    bne     drt
    sta     ANY_DIRTY

    ;; exit
    lda     track
    ldx     #0

    rts

_disk_swap:
    lda     #WRITE_PROT
    sta     GPIO_CLEAR

    ;; store swap delay
    jsr     popa
    sta     swap_delay

    ;; store write protect flag
    jsr     popa
    pha
    jmp     disk_done
    
    .export     _disk_swap

.endproc

