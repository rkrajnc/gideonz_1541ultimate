	.setcpu		"6502"
	.smart		on
	.autoimport	on
	.case		on
	.debuginfo	off
	
	.importzp   bankedFp		; the address of the function we actually will be banking to
	.import		SwitchBank		; the function doing the actual switch
	
	; the banked functions
	.import		_banked_dir_change
	.import		_banked_dir_getentry	
	.import		_banked_dir_getindex	
	.import		_banked_dir_analyze_name
	.import		_banked_dir_match_name
	.import		_banked_dir_find_entry
	.import		_banked_dump_entryname

	; the functions that this file provides a gateway to
	.export		_dir_change
	.export		_dir_getentry	
	.export		_dir_getindex	
	.export		_dir_analyze_name
	.export		_dir_match_name
	.export		_dir_find_entry
	.export		_dump_entryname

_dir_change:		
	    lda     #>(_banked_dir_change)
	    sta     bankedFp+1
    	lda     #<(_banked_dir_change)
	    sta     bankedFp
		jmp		SwitchBank	
_dir_getentry:	
	    lda     #>(_banked_dir_getentry)
	    sta     bankedFp+1
    	lda     #<(_banked_dir_getentry)
	    sta     bankedFp
		jmp		SwitchBank	
_dir_getindex:			
	    lda     #>(_banked_dir_getindex)
	    sta     bankedFp+1
    	lda     #<(_banked_dir_getindex)
	    sta     bankedFp
		jmp		SwitchBank		        
_dir_analyze_name:
	    lda     #>(_banked_dir_analyze_name)
	    sta     bankedFp+1
    	lda     #<(_banked_dir_analyze_name)
	    sta     bankedFp
		jmp		SwitchBank	
_dir_match_name:		
	    lda     #>(_banked_dir_match_name)
	    sta     bankedFp+1
    	lda     #<(_banked_dir_match_name)
	    sta     bankedFp
		jmp		SwitchBank	
_dir_find_entry:		
	    lda     #>(_banked_dir_find_entry)
	    sta     bankedFp+1
    	lda     #<(_banked_dir_find_entry)
	    sta     bankedFp
		jmp		SwitchBank	

_dump_entryname:		
	    lda     #>(_banked_dump_entryname)
	    sta     bankedFp+1
    	lda     #<(_banked_dump_entryname)
	    sta     bankedFp
		jmp		SwitchBank	
				