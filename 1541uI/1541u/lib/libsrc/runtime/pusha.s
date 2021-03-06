.segment "LIBCODE"
;
; Ullrich von Bassewitz, 26.10.2000
;
; CC65 runtime: Push value in a onto the stack
;

       	.export	  	pusha0sp, pushaysp, pusha
	.importzp	sp

        .macpack        cpu

; Beware: The optimizer knows about this function!

pusha0sp:
	ldy	#$00
pushaysp:
	lda	(sp),y
pusha:	ldy	sp              ; (3)
       	beq	@L1             ; (6)
 	dec	sp              ; (11)
.if (.cpu .bitand CPU_ISET_65SC02)
	sta	(sp)
.else
    	ldy	#0              ; (13)
    	sta	(sp),y          ; (19)
.endif
    	rts                     ; (25)

@L1:	dec	sp+1            ; (11)
    	dec	sp              ; (16)
    	sta	(sp),y          ; (22)
    	rts                     ; (28)

