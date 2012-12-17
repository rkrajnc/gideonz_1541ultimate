;-------------------------------------------------------------------------------
;--
;--  (C) COPYRIGHT 2007, Gideon's Logic Architectures
;--
;-------------------------------------------------------------------------------
;-- Project    : Ultimate 1541
;-- Title      : asciipetscii.asm
;-------------------------------------------------------------------------------
;-- Abstract:
;	Converting fromascii 2 petscii and from petscii 2 ascii
;-- 
;------------------------------------------------------------------------------- 

; void ascii2petscii (const char* s1, const char* s2);

 	.export		_ascii2petscii
 	.export		_petscii2ascii
	.import		popax
	.importzp	ptr1, ptr2, tmp1

    .segment    "FASTCODE"

_ascii2petscii:
     	jsr	popax 	       	; get s2
     	sta	ptr2	       	; Save s2
     	stx	ptr2+1
     	jsr	popax 	       	; get s1
     	sta	ptr1
     	stx	ptr1+1
     	ldy	#0

_aploop:lda		(ptr2),y   	; get char from second string (ascii)
		cmp     #$61
		bcc     _apcaps		; if s2[y] >= 'a'
		cmp     #$7B		; 					&& s2[y] <= 'z'
		bcs     _apcaps
		sec
		sbc     #$20		; dec 0x20 --> s1[y] = s2[y] - 0x20;
		jmp		_apstor
		
_apcaps:cmp     #$41
		bcc     _apspec		; if s2[y] >= 'A'
		cmp     #$5B		; 					&& s2[y] <= 'A'
		bcs     _apspec
		clc
		adc     #$80		; add 0x20 --> s1[y] = s2[y] + 0x20;

_apspec:cmp     #$5F        ; _
        bne     _apstor
        lda     #$A4
        
_apstor:sta     (ptr1),y	; 	
		cmp		#0			; if s2[y] == 0
		beq		_apend		;	goto end
	  	iny					; y++
  		bne		_aploop		; if(y!=0) goto loop
	  	inc		ptr1+1		; s1 higher byte ++
  		inc		ptr2+1		; s2 higher byte ++
	  	bne		_aploop		; goto loop
_apend:	rts



_petscii2ascii:
     	jsr	popax 	       	; get s2
     	sta	ptr2	       	; Save s2
     	stx	ptr2+1
     	jsr	popax 	       	; get s1
     	sta	ptr1
     	stx	ptr1+1
     	ldy	#0

_paloop:lda		(ptr2),y   	; get char from second string (petscii)
		cmp     #$41
		bcc     _pacaps		; if s2[y] >= 'a'
		cmp     #$5B		; 					&& s2[y] <= 'z'
		bcs     _pacaps
		clc
		adc     #$20		; add 0x20 --> s1[y] = s2[y] + 0x20;
		jmp		_pastor
		
_pacaps:cmp     #$C1
		bcc     _paspec		; if s2[y] >= 'A'
		cmp     #$DB		; 					&& s2[y] <= 'A'
		bcs     _paspec
		sec
		sbc     #$80		; dec 0x20 --> s1[y] = s2[y] - 0x20;

_paspec:cmp     #$A4
        bne     _pastor
        lda     #$5F
        
_pastor:sta     (ptr1),y	; 	
		cmp		#0			; if s2[y] == 0
		beq		_paend		;	goto end
	  	iny					; y++
  		bne		_paloop		; if(y!=0) goto loop
	  	inc		ptr1+1		; s1 higher byte ++
  		inc		ptr2+1		; s2 higher byte ++
	  	bne		_paloop		; goto loop
_paend:	rts
		
