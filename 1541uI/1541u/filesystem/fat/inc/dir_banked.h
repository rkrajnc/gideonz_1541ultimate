#ifndef _BANKED_DIR_H
#define _BANKED_DIR_H

/* INCLUDE ONLY when you reside in the same bank as dir.c */
#define dir_change			banked_dir_change
#define dir_getentry		banked_dir_getentry
#define dir_getindex		banked_dir_getindex
#define dir_analyze_name	banked_dir_analyze_name
#define dir_match_name		banked_dir_match_name
#define dir_find_entry		banked_dir_find_entry

#define dump_entryname		banked_dump_entryname
#endif

