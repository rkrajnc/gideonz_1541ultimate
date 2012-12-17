#ifndef _BANKED_D64_H
#define _BANKED_D64_H

/* INCLUDE ONLY when you reside in the same bank as d64.c */

#define d64_dir 		banked_d64_dir
#define d64_readblock	banked_d64_readblock
#define d64_loadfile	banked_d64_loadfile
#define d64_write_track banked_d64_write_track
#define d64_create		banked_d64_create
#define d64_save		banked_d64_save
#define d64_load_params banked_d64_load_params

#endif

