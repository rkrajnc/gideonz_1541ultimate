	.setcpu		"6502"
	.smart		on
	.autoimport	on
	.case		on
	.debuginfo	off
	
	.importzp   bankedFp		; the address of the function we actually will be banking to
	.import		SwitchBank		; the function doing the actual switch
	
	; the banked functions
	.import		_banked_t64_dir		
	.import		_banked_t64_readblock
	.import		_banked_t64_loadfile
	.import		_banked_t64_loadfirst

	; the functions that this file provides a gateway to
	.export		_t64_dir		
	.export		_t64_readblock
	.export		_t64_loadfile
	.export		_t64_loadfirst

_t64_dir:		
	    lda     #>(_banked_t64_dir)
	    sta     bankedFp+1
    	lda     #<(_banked_t64_dir)
	    sta     bankedFp
		jmp		SwitchBank	
_t64_readblock:	
	    lda     #>(_banked_t64_readblock)
	    sta     bankedFp+1
    	lda     #<(_banked_t64_readblock)
	    sta     bankedFp
		jmp		SwitchBank	
_t64_loadfile:			
	    lda     #>(_banked_t64_loadfile)
	    sta     bankedFp+1
    	lda     #<(_banked_t64_loadfile)
	    sta     bankedFp
		jmp		SwitchBank		    
_t64_loadfirst:			
	    lda     #>(_banked_t64_loadfirst)
	    sta     bankedFp+1
    	lda     #<(_banked_t64_loadfirst)
	    sta     bankedFp
		jmp		SwitchBank		    

