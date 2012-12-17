	.setcpu		"6502"
	.smart		on
	.autoimport	on
	.case		on
	.debuginfo	off
	
	.importzp   bankedFp		; the address of the function we actually will be banking to
	.import		SwitchBank		; the function doing the actual switch
	
	; the banked functions
	.import		_banked_f_open
	.import		_banked_f_open_direct
	.import		_banked_f_read
	.import		_banked_f_close
	.import		_banked_f_lseek
	.import		_banked_f_opendir
	.import		_banked_f_opendir_direct
	.import		_banked_f_readdir

	.import		_banked_f_stat
	.import		_banked_f_getfree
	.import		_banked_f_mountdrv
	.import		_banked_f_write
	.import		_banked_f_sync
	.import		_banked_f_unlink
	.import		_banked_f_mkdir
	.import		_banked_f_chmod
	.import		_banked_f_rename
	.import		_banked_f_create_file_entry
	.import		_banked_f_create_dir_entry
	.import		_banked_f_unlink_entry
	.import		_banked_f_unlink_direct
	.import		_banked_f_rename_direct
    .import     _sd_writeSector

	; the functions that this file provides a gateway to
	.export		_f_open			
	.export		_f_open_direct	
	.export		_f_read			
	.export		_f_close			
	.export		_f_lseek			
	.export		_f_opendir		
	.export		_f_opendir_direct
	.export		_f_readdir		

	.export		_f_stat			
	.export		_f_getfree		
	.export		_f_mountdrv		
	.export		_f_write			
	.export		_f_sync			
	.export		_f_unlink		
	.export		_f_mkdir			
	.export		_f_chmod			
	.export		_f_rename		
	.export		_f_create_file_entry	
	.export		_f_create_dir_entry	
	.export		_f_unlink_entry
	.export		_f_unlink_direct
	.export     _f_rename_direct
    .export     _bank1_sd_writeSector
	
_f_open:		
	    lda     #>(_banked_f_open)
	    sta     bankedFp+1
    	lda     #<(_banked_f_open)
	    sta     bankedFp
		jmp		SwitchBank	
_f_open_direct:	
	    lda     #>(_banked_f_open_direct)
	    sta     bankedFp+1
    	lda     #<(_banked_f_open_direct)
	    sta     bankedFp
		jmp		SwitchBank	
_f_read:			
	    lda     #>(_banked_f_read)
	    sta     bankedFp+1
    	lda     #<(_banked_f_read)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_close:			
	    lda     #>(_banked_f_close)
	    sta     bankedFp+1
    	lda     #<(_banked_f_close)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_lseek:			
	    lda     #>(_banked_f_lseek)
	    sta     bankedFp+1
    	lda     #<(_banked_f_lseek)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_opendir:		
	    lda     #>(_banked_f_opendir)
	    sta     bankedFp+1
    	lda     #<(_banked_f_opendir)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_opendir_direct:
	    lda     #>(_banked_f_opendir_direct)
	    sta     bankedFp+1
    	lda     #<(_banked_f_opendir_direct)
	    sta     bankedFp
   		jmp		SwitchBank	
_f_readdir:		
	    lda     #>(_banked_f_readdir)
	    sta     bankedFp+1
    	lda     #<(_banked_f_readdir)
	    sta     bankedFp
		jmp		SwitchBank		    	    
_f_stat:			
	    lda     #>(_banked_f_stat)
	    sta     bankedFp+1
    	lda     #<(_banked_f_stat)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_getfree:		
	    lda     #>(_banked_f_getfree)
	    sta     bankedFp+1
    	lda     #<(_banked_f_getfree)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_mountdrv:		
	    lda     #>(_banked_f_mountdrv)
	    sta     bankedFp+1
    	lda     #<(_banked_f_mountdrv)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_write:			
	    lda     #>(_banked_f_write)
	    sta     bankedFp+1
    	lda     #<(_banked_f_write)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_sync:			
	    lda     #>(_banked_f_sync)
	    sta     bankedFp+1
    	lda     #<(_banked_f_sync)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_unlink:		
	    lda     #>(_banked_f_unlink)
	    sta     bankedFp+1
    	lda     #<(_banked_f_unlink)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_mkdir:			
	    lda     #>(_banked_f_mkdir)
	    sta     bankedFp+1
    	lda     #<(_banked_f_mkdir)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_chmod:			
	    lda     #>(_banked_f_chmod)
	    sta     bankedFp+1
    	lda     #<(_banked_f_chmod)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_rename:		
	    lda     #>(_banked_f_rename)
	    sta     bankedFp+1
    	lda     #<(_banked_f_rename)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_create_file_entry:	
	    lda     #>(_banked_f_create_file_entry)
	    sta     bankedFp+1
    	lda     #<(_banked_f_create_file_entry)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_create_dir_entry:	
	    lda     #>(_banked_f_create_dir_entry)
	    sta     bankedFp+1
    	lda     #<(_banked_f_create_dir_entry)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_unlink_entry:	
	    lda     #>(_banked_f_unlink_entry)
	    sta     bankedFp+1
    	lda     #<(_banked_f_unlink_entry)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_unlink_direct:	
	    lda     #>(_banked_f_unlink_direct)
	    sta     bankedFp+1
    	lda     #<(_banked_f_unlink_direct)
	    sta     bankedFp
		jmp		SwitchBank		    
_f_rename_direct:	
	    lda     #>(_banked_f_rename_direct)
	    sta     bankedFp+1
    	lda     #<(_banked_f_rename_direct)
	    sta     bankedFp
		jmp		SwitchBank		    
_bank1_sd_writeSector:		
	    lda     #>(_sd_writeSector)
	    sta     bankedFp+1
    	lda     #<(_sd_writeSector)
	    sta     bankedFp
		jmp		SwitchBank	

