	.setcpu		"6502"
	.smart		on
	.autoimport	on
	.case		on
	.debuginfo	on
	.importzp	sp, sreg, regsave, regbank, tmp1, ptr1, ptr2, ptr3, ptr4
	.exportzp	_FatFs_fs_type
	.exportzp	_FatFs_files
	.exportzp	_FatFs_sects_clust
	.exportzp	_FatFs_n_fats
	.exportzp	_FatFs_n_rootdir
	.exportzp	_FatFs_winflag
	.exportzp	_FatFs_winsect
	.exportzp	_FatFs_sects_fat
	.exportzp	_FatFs_max_clust
	.exportzp	_FatFs_fatbase
	.exportzp	_FatFs_dirbase
	.exportzp	_FatFs_database
	.exportzp	_FatFs_last_clust
	.export		_FatFs_win
   
.segment	"ZEROPAGE"

_FatFs_fs_type: 	.res	1,$00
_FatFs_files:   	.res	1,$00
_FatFs_sects_clust:	.res	1,$00
_FatFs_n_fats: 	    .res	1,$00
_FatFs_n_rootdir:	.res	2,$00
_FatFs_winflag:	    .res	1,$00
_FatFs_winsect:	    .res	4,$00
_FatFs_sects_fat:	.res	4,$00
_FatFs_max_clust:	.res	4,$00
_FatFs_fatbase:	    .res	4,$00
_FatFs_dirbase:	    .res	4,$00
_FatFs_database:	.res	4,$00
_FatFs_last_clust:	.res	4,$00

.segment	"BSS"

_FatFs_win:	        .res	512,$00

