;
; Ullrich von Bassewitz, 15.09.2000
;
; int memcmp (const void* p1, const void* p2, size_t count);
;

	.export		_memcmp
	.import		popax, return0
	.importzp	ptr1, ptr2, ptr3

.segment "LIBCODE"
_memcmp:

; Calculate (-count-1) and store it into ptr3. This is some overhead here but
; saves time in the compare loop

	eor	#$FF
	sta	ptr3
	txa
	eor	#$FF
	sta	ptr3+1

; Get the pointer parameters

       	jsr	popax 	       	; Get p2
    	sta	ptr2
    	stx	ptr2+1
    	jsr	popax 		; Get p1
    	sta	ptr1
    	stx	ptr1+1

; Loop initialization

     	ldx	ptr3		; Load low counter byte into X
     	ldy	#$00		; Initialize pointer

; Head of compare loop: Test for the end condition

Loop:	inx	      		; Bump low byte of (-count-1)
       	beq	BumpHiCnt	; Jump on overflow

; Do the compare

Comp:	lda	(ptr1),y
	cmp	(ptr2),y
	bne	NotEqual	; Jump if bytes not equal

; Bump the pointers

	iny	   		; Increment pointer
	bne	Loop
	inc	ptr1+1		; Increment high bytes
	inc	ptr2+1
	bne	Loop		; Branch always (pointer wrap is illegal)

; Entry on low counter byte overflow

BumpHiCnt:
    	inc	ptr3+1		; Bump high byte of (-count-1)
       	bne	Comp		; Jump if not done
	jmp	return0		; Count is zero, areas are identical

; Not equal, check which one is greater

NotEqual:
	bcs	Greater
  	ldx	#$FF		; Make result negative
  	rts

Greater:
	ldx	#$01		; Make result positive
	rts

