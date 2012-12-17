	.setcpu		"6502"
	.smart		on
	.autoimport	on
	.case		on
	.debuginfo	off
	
	.importzp   bankedFp		; the address of the function we actually will be banking to
	.import		SwitchBank		; the function doing the actual switch
	
	; the banked functions
	.import		_banked_d64_dir 		
	.import		_banked_d64_readblock	
	.import		_banked_d64_loadfile	
	.import		_banked_d64_write_track
	.import		_banked_d64_create		
	.import		_banked_d64_save		
    .import     _banked_d64_load_params
    
	; the functions that this file provides a gateway to
	.export		_d64_dir 		
	.export		_d64_readblock	
	.export		_d64_loadfile	
	.export		_d64_write_track
	.export		_d64_create		
	.export		_d64_save		
	.export     _d64_load_params
	
_d64_dir:		
	    lda     #>(_banked_d64_dir)
	    sta     bankedFp+1
    	lda     #<(_banked_d64_dir)
	    sta     bankedFp
		jmp		SwitchBank	
_d64_readblock:	
	    lda     #>(_banked_d64_readblock)
	    sta     bankedFp+1
    	lda     #<(_banked_d64_readblock)
	    sta     bankedFp
		jmp		SwitchBank	
_d64_loadfile:			
	    lda     #>(_banked_d64_loadfile)
	    sta     bankedFp+1
    	lda     #<(_banked_d64_loadfile)
	    sta     bankedFp
		jmp		SwitchBank		    
_d64_write_track:			
	    lda     #>(_banked_d64_write_track)
	    sta     bankedFp+1
    	lda     #<(_banked_d64_write_track)
	    sta     bankedFp
		jmp		SwitchBank		    
_d64_create:			
	    lda     #>(_banked_d64_create)
	    sta     bankedFp+1
    	lda     #<(_banked_d64_create)
	    sta     bankedFp
		jmp		SwitchBank		    
_d64_save:
	    lda     #>(_banked_d64_save)
	    sta     bankedFp+1
    	lda     #<(_banked_d64_save)
	    sta     bankedFp
		jmp		SwitchBank		    
_d64_load_params:
	    lda     #>(_banked_d64_load_params)
	    sta     bankedFp+1
    	lda     #<(_banked_d64_load_params)
	    sta     bankedFp
		jmp		SwitchBank		    
