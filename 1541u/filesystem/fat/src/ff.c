/*
 *   1541 Ultimate - The Storage Solution for your Commodore computer.
 *   Copyright (C) 2009  Gideon Zweijtzer - Gideon's Logic Architectures
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Abstract:
 *	 Fat file system implementation. Adapted for use with the 65GZ02.
 *
/---------------------------------------------------------------------------/
/  FatFs - FAT file system module  R0.03a                    (C)ChaN, 2006
/---------------------------------------------------------------------------/
/ FatFs module is an experimenal project to implement FAT file system to
/ cheap microcontrollers. This is a free software and is opened for education,
/ research and development under license policy of following trems.
/
/  Copyright (C) 2006, ChaN, all right reserved.
/
/ * The FatFs module is a free software and there is no warranty.
/ * You can use, modify and/or redistribute it for personal, non-profit or
/   profit use without any restriction under your responsibility.
/ * Redistributions of source code must retain the above copyright notice.
/
/---------------------------------------------------------------------------/
/  Feb 26, 2006  R0.00  Prototype.
/  Apr 29, 2006  R0.01  First stable version.
/  Jun 01, 2006  R0.02  Added FAT12. Removed unbuffered mode.
/                       Fixed a problem on small (<32M) patition.
/  Jun 10, 2006  R0.02a Added a configuration option (_FS_MINIMUM).
/  Sep 22, 2006  R0.03  Added f_rename().
/                       Changed option _FS_MINIMUM to _FS_MINIMIZE.
/  Dec 11, 2006  R0.03a Improved cluster scan algolithm to write files fast.
/                       Fixed f_mkdir() creates incorrect directory on FAT32.
/---------------------------------------------------------------------------*/
#include "manifest.h"
#include <string.h>
#include <stdio.h>

#ifndef NO_BANKING
#include "dir_banked.h"
#include "ff_banked.h"	/* Renames all functions here so they can reside in a seperate bank */
#endif

#include "ff.h"			/* FatFs declarations */
#include "dir.h"
#include "diskio.h"		/* Include file for user provided disk functions */
#include "gpio.h"
#include "data_tools.h"

#include "dump_hex.h"

#pragma codeseg ("FILESYS")

//FATFS FatFs;			/* File system object */

/* User defined function to give a current time to fatfs module */

DWORD get_fattime(void);    /* 31-25: Year(0-127 +1980), 24-21: Month(1-12), 20-16: Day(1-31) */
                            /* 15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2) */


#ifdef _FS_LFN
static char temp_name[MAX_ENTRY_NAME+1]; // one longer to see when a filename is longer than we can interpret

static DIR firstscan, // containing info to guide to the first entry of a lfn
		   lastscan,  // containing info to guide to the last entry of a lfn
		   tempscan;  // just a tramp. Use it for whatever you like.

#endif /* _FS_LFN */

/*-------------------------------------------------------------------------
  Module Private Functions
-------------------------------------------------------------------------*/

/*----------------------*/
/* Change Window Offset */

BOOL move_window (
	DWORD sector		/* Sector number to make apperance in the FatFs->win */
)						/* Move to zero only writes back dirty window */
{
	DWORD wsect;

	wsect = FatFs_winsect;

//    printf("Move Window from %ld to %ld.\n", wsect, sector);

	if (wsect != sector) {	/* Changed current window */
#ifndef _FS_READONLY
		BYTE n;
		if (FatFs_winflag) {	/* Write back dirty window if needed */
			if (disk_write(FatFs_win, wsect, 1) != RES_OK) return FALSE;
			FatFs_winflag = 0;
			if (wsect < (FatFs_fatbase + FatFs_sects_fat)) {	/* In FAT area */
				for (n = FatFs_n_fats; n >= 2; n--) {	/* Refrect the change to all FAT copies */
					wsect += FatFs_sects_fat;
					if (disk_write(FatFs_win, wsect, 1) != RES_OK) break;
				}
			}
		}
#endif
		if (sector) {
			if (disk_read(FatFs_win, sector, 1) != RES_OK) return FALSE;
			FatFs_winsect = sector;
		}
	}
	return TRUE;
}



/*----------------------*/
/* Get a Cluster Status */

DWORD get_cluster (
	DWORD clust			// Cluster# to get the link information
)
{
#ifdef _FS_FAT12
	WORD wc, bc;
#endif
	DWORD fatsect;

	if ((clust >= 2) && (clust < FatFs_max_clust)) {		// Valid cluster#
		fatsect = FatFs_fatbase;
		switch (FatFs_fs_type) {
#ifdef _FS_FAT12
		case FS_FAT12 :
			bc = (WORD)clust * 3 / 2;
			if (!move_window(fatsect + bc / 512)) break;
			wc = FatFs_win[bc % 512]; bc++;
			if (!move_window(fatsect + bc / 512)) break;
			wc |= (WORD)FatFs_win[bc % 512] << 8;
			return (clust & 1) ? (wc >> 4) : (wc & 0xFFF);
#endif
		case FS_FAT16 :
			if (!move_window(fatsect + clust / 256)) break;
			return LD_WORD(&(FatFs_win[((WORD)clust * 2) % 512]));

		case FS_FAT32 :
			if (!move_window(fatsect + clust / 128)) break;
			return LD_DWORD(&(FatFs_win[((WORD)clust * 4) % 512])) & 0x0FFFFFFF;
		}
	}
	return 1;	// There is no cluster information, or an error occured 
}

/*--------------------------*/
/* Change a Cluster Status  */

#ifndef _FS_READONLY
BOOL put_cluster (
	DWORD clust,		/* Cluster# to change */
	DWORD val			/* New value to mark the cluster */
)
{
#ifdef _FS_FAT12
	WORD bc;
	BYTE *p;
#endif
	DWORD fatsect;

	fatsect = FatFs_fatbase;
	switch (FatFs_fs_type) {
#ifdef _FS_FAT12
	case FS_FAT12 :
		bc = (WORD)clust * 3 / 2;
		if (!move_window(fatsect + bc / 512)) return FALSE;
		p = &FatFs_win[bc % 512];
		*p = (clust & 1) ? ((*p & 0x0F) | ((BYTE)val << 4)) : (BYTE)val;
		bc++;
		FatFs_winflag = 1; 
		if (!move_window(fatsect + bc / 512)) return FALSE;
		p = &FatFs_win[bc % 512];
		*p = (clust & 1) ? (BYTE)(val >> 4) : ((*p & 0xF0) | ((BYTE)(val >> 8) & 0x0F));
		break;
#endif
	case FS_FAT16 :
		if (!move_window(fatsect + clust / 256)) return FALSE;
		ST_WORD(&(FatFs_win[((WORD)clust * 2) % 512]), (WORD)val);
		break;

	case FS_FAT32 :
		if (!move_window(fatsect + clust / 128)) return FALSE;
		ST_DWORD(&(FatFs_win[((WORD)clust * 4) % 512]), val);
		break;

	default :
		return FALSE;
	}
	FatFs_winflag = 1;
	return TRUE;
}
#endif /* _FS_READONLY */



/*------------------------*/
/* Remove a Cluster Chain */

#ifndef _FS_READONLY
BOOL remove_chain (
	DWORD clust			/* Cluster# to remove chain from */
)
{
	DWORD nxt;


	if (clust) {
		while ((nxt = get_cluster(clust)) >= 2) {
			if (!put_cluster(clust, 0)) return FALSE;
			clust = nxt;
		}
	}
	return TRUE;
}
#endif



/*-----------------------------------*/
/* Stretch or Create a Cluster Chain */

#ifndef _FS_READONLY
DWORD create_chain (
	DWORD clust			 /* Cluster# to stretch, 0 means create new */
)
{
	DWORD cstat, ncl, scl, mcl;

	mcl = FatFs_max_clust;
	if (clust == 0) {		/* Create new chain */
		scl = FatFs_last_clust;			/* Get last allocated cluster */
		if (scl < 2 || scl >= mcl) scl = 1;
	}
	else {					/* Stretch existing chain */
		cstat = get_cluster(clust);		/* Check the cluster status */
		if (cstat < 2) return 0;		/* It is an invalid cluster */
		if (cstat < mcl) return cstat;	/* It is already followed by next cluster */
		scl = clust;
	}
	ncl = scl;				/* Scan start cluster */
	do {
		ncl++;						/* Next cluster */
		if (ncl >= mcl) {			/* Wrap around */
			ncl = 2;
			if (scl == 1) return 0;	/* No free cluster was found */
		}
		if (ncl == scl) return 0;	/* No free cluster was found */
		cstat = get_cluster(ncl);	/* Get the cluster status */
		if (cstat == 1) return 0;	/* Any error occured */
	} while (cstat);				/* Repeat until find a free cluster */

	if (!put_cluster(ncl, 0x0FFFFFFF)) return 0;		/* Mark the new cluster "in use" */
	if (clust && !put_cluster(clust, ncl)) return 0;	/* Link it to previous one if needed */
	FatFs_last_clust = ncl;

	return ncl;		/* Return new cluster number */
}
#endif /* _FS_READONLY */



/*----------------------------*/
/* Get Sector# from Cluster#  */

#pragma codeseg ("LIBCODE")
DWORD clust2sect (
	DWORD clust		/* Cluster# to be converted */
)
{

	clust -= 2;
	if (clust >= FatFs_max_clust) return 0;		/* Invalid cluster# */
	return clust * FatFs_sects_clust + FatFs_database;
}

#pragma codeseg ("FILESYS")



/*------------------------*/
/* Check File System Type */

BYTE check_fs (
	DWORD sect		/* Sector# to check if it is a FAT boot record or not */
)
{
	static const CHAR fatsign[] = { 0x46, 0x41, 0x54, 0x31, 0x32,
		                            0x46, 0x41, 0x54, 0x31, 0x36,
		                            0x46, 0x41, 0x54, 0x33, 0x32 };    /* "FAT12FAT16FAT32"; <- doesn't work with CC65 (PETASC?) */
	/* Determines FAT type by signature string but this is not correct.
	   For further information, refer to fatgen103.doc from Microsoft. */
	memset(FatFs_win, 0, 512);
	if (disk_read(FatFs_win, sect, 1) == RES_OK) {	/* Load boot record */
		if (LD_WORD(&(FatFs_win[510])) == 0xAA55) {		/* Is it valid? */
/*			printf("Valid..."); */
#ifdef _FS_FAT12
			if (!memcmp(&(FatFs_win[0x36]), &fatsign[0], 5))
				return FS_FAT12;
#endif
			if (!memcmp(&(FatFs_win[0x36]), &fatsign[5], 5))
				return FS_FAT16;
			if (!memcmp(&(FatFs_win[0x52]), &fatsign[10], 5) && (FatFs_win[0x28] == 0))
				return FS_FAT32;
/*			printf(" but signature not recognized.\n"); */
		}
	}
	return 0;
}


/*--------------------------------*/
/* Move Directory Pointer to Next */

BOOL next_dir_entry (
	DIR *scan			/* Pointer to directory object */
)
{
	DWORD clust;
	WORD idx;

	idx = scan->index + 1;
	if ((idx & 15) == 0) {		/* Table sector changed? */
		scan->sect++;			/* Next sector */
		if (!scan->clust) {		/* In static table */
			if (idx >= FatFs_n_rootdir) return FALSE;	/* Reached to end of table */
		} else {				/* In dynamic table */
			if (((idx / 16) & (FatFs_sects_clust - 1)) == 0) {	/* Cluster changed? */
				clust = get_cluster(scan->clust);		/* Get next cluster */
				if ((clust >= FatFs_max_clust) || (clust < 2))	/* Reached to end of table */
					return FALSE;
				scan->clust = clust;				/* Initialize for new cluster */
				scan->sect = clust2sect(clust);
			}
		}
	}
	scan->index = idx;	/* Lower 4 bit of scan->index indicates offset in scan->sect */
	return TRUE;
}



/*--------------------------------------*/
/* Get File Status from Directory Entry */

#if _FS_MINIMIZE <= 1
void get_fileinfo (
	FILINFO *finfo, 	/* Ptr to store the file information */
	const BYTE *dir		/* Ptr to the directory entry */
)
{
	BYTE n, c, a;
	CHAR *p;

	p = &(finfo->fname[0]);
	a = *(dir+12);	/* NT flag */
	for (n = 0; n < 8; n++) {	/* Convert file name (body) */
		c = *(dir+n);
		if (c == ' ') break;
		if (c == 0x05) c = 0xE5;
		if ((a & 0x08) && (c >= 'A') && (c <= 'Z')) c += 0x20;
		*p++ = c;
	}
	if (*(dir+8) != ' ') {		/* Convert file name (extension) */
		*p++ = '.';
		for (n = 8; n < 11; n++) {
			c = *(dir+n);
			if (c == ' ') break;
			if ((a & 0x10) && (c >= 'A') && (c <= 'Z')) c += 0x20;
			*p++ = c;
		}
	}
	*p = '\0';

	finfo->fattrib  = *(dir+11);			    /* Attribute */
    finfo->fcluster = ((DWORD)LD_WORD(dir+20) << 16) | LD_WORD(dir+26); /* Cluster */
	finfo->fsize    = LD_DWORD(dir+28);	        /* Size */
	finfo->fdate    = LD_WORD(dir+24);		    /* Date */
	finfo->ftime    = LD_WORD(dir+22);		    /* Time */
}
#endif /* _FS_MINIMIZE <= 1 */

#ifdef _FS_LFN
BYTE calc_lfn_checksum (
	const BYTE *dir		/* Ptr to the directory entry */
)
{
    static BYTE sum;
    static BYTE i;
    
    for (i=0, sum=0; i!=11; i++)
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + dir[i];

    return sum;
}
#endif /* _FS_LFN */

/*-------------------------------------------------------------------*/
/* Pick a Paragraph and Create the Name in Format of Directory Entry */

CHAR make_dirfile (
	const CHAR **path,		/* Pointer to the file path pointer */
	CHAR *dirname			/* Pointer to directory name buffer {Name(8), Ext(3), NT flag(1)} */
)
{
	BYTE n, t, c, a, b;


	memset(dirname, ' ', 8+3);	/* Fill buffer with spaces */
	a = 0; b = 0x18;	/* NT flag */
	n = 0; t = 8;
	for (;;) {
		c = *(*path)++;
		if (c <= ' ') c = 0;
		if ((c == 0) || (c == '/')) {			/* Reached to end of str or directory separator */
			if (n == 0) break;
			dirname[11] = a & b; return c;
		}
		if (c == '.') {
			if(!(a & 1) && (n >= 1) && (n <= 8)) {	/* Enter extension part */
				n = 8; t = 11; continue;
			}
			break;
		}
#ifdef _USE_SJIS
		if (((c >= 0x81) && (c <= 0x9F)) ||		/* Accept S-JIS code */
		    ((c >= 0xE0) && (c <= 0xFC))) {
			if ((n == 0) && (c == 0xE5))		/* Change heading \xE5 to \x05 */
				c = 0x05;
			a ^= 1; goto md_l2;
		}
		if ((c >= 0x7F) && (c <= 0x80)) break;	/* Reject \x7F \x80 */
#else
		if (c >= 0x7F) goto md_l1;				/* Accept \x7F-0xFF */
#endif
		if (c == '"') break;					/* Reject " */
		if (c <= ')') goto md_l1;				/* Accept ! # $ % & ' ( ) */
		if (c <= ',') break;					/* Reject * + , */
		if (c <= '9') goto md_l1;				/* Accept - 0-9 */
		if (c <= '?') break;					/* Reject : ; < = > ? */
		if (!(a & 1)) {	/* These checks are not applied to S-JIS 2nd byte */
			if (c == '|') break;				/* Reject | */
			if ((c >= '[') && (c <= ']')) break;/* Reject [ \ ] */
			if ((c >= 'A') && (c <= 'Z'))
				(t == 8) ? (b &= ~0x08) : (b &= ~0x10);
			if ((c >= 'a') && (c <= 'z')) {		/* Convert to upper case */
				c -= 0x20;
				(t == 8) ? (a |= 0x08) : (a |= 0x10);
			}
		}
	md_l1:
		a &= ~1;
#ifdef _USE_SJIS
	md_l2:
#endif
		if (n >= t) break;
		dirname[n++] = c;
	}
	return 1;
}

/*-------------------*/
/* Trace a File Path */

FRESULT trace_path (
	DIR *scan,			/* Pointer to directory object to return last directory */
	CHAR *fn,			/* Pointer to last segment name to return */
	const CHAR *path,	/* Full-path string to trace a file or directory */
	BYTE **dir			/* Directory pointer in Win[] to retutn */
)
{
	DWORD clust;
	CHAR ds;
	BYTE *dptr = NULL;

	/* Initialize directory object */
	clust = FatFs_dirbase;
	if (FatFs_fs_type == FS_FAT32) {
		scan->clust = scan->sclust = clust;
		scan->sect = clust2sect(clust);
	} else {
		scan->clust = scan->sclust = 0;
		scan->sect = clust;
	}
	scan->index = 0;

	while ((*path == ' ') || (*path == '/')) path++;	/* Skip leading spaces */
	if ((BYTE)*path < ' ') {							/* Null path means the root directory */
		*dir = NULL; return FR_OK;
	}

	for (;;) {
		ds = make_dirfile(&path, fn);			/* Get a paragraph into fn[] */
		if (ds == 1) return FR_INVALID_NAME;
		for (;;) {
			if (!move_window(scan->sect)) return FR_RW_ERROR;
			dptr = &(FatFs_win[(scan->index & 15) * 32]);	/* Pointer to the directory entry */
			if (*dptr == 0)								/* Has it reached to end of dir? */
				return !ds ? FR_NO_FILE : FR_NO_PATH;
			if (    (*dptr != 0xE5)						/* Matched? */
				&& !(*(dptr+11) & AM_VOL)
				&& !memcmp(dptr, fn, 8+3) ) break;
			if (!next_dir_entry(scan))					/* Next directory pointer */
				return !ds ? FR_NO_FILE : FR_NO_PATH;
		}
		if (!ds) { *dir = dptr; return FR_OK; }			/* Matched with end of path */
		if (!(*(dptr+11) & AM_DIR)) return FR_NO_PATH;	/* Cannot trace because it is a file */
		clust = ((DWORD)LD_WORD(dptr+20) << 16) | LD_WORD(dptr+26); /* Get cluster# of the directory */
		scan->clust = scan->sclust = clust;				/* Restart scan with the new directory */
		scan->sect = clust2sect(clust);
		scan->index = 0;
	}
}

/*---------------------------*/
/* Reserve a Directory Entry */

#ifndef _FS_READONLY
BYTE* reserve_direntry (
	DIR *scan,		/* Target directory to create new entry */
    WORD offset     /* search start */
)
{
	DWORD clust, sector;
	BYTE c, n, *dptr;

    /* offset=0 means that we will search from the beginning, hence
       we do a reinit of the directory object. If the offset > 0, we
       assume that we're doing that search in the same dir as before,
       so we only do a re-init if the requested offset is smaller
       than the current index. */

    if((offset == 0)||(offset < scan->index)) {
    	/* Re-initialize directory object */
    	clust = scan->sclust;
    	if (clust) {	/* Dyanmic directory table */
    		scan->clust = clust;
    		scan->sect = clust2sect(clust);
    	} else {		/* Static directory table */
    		scan->sect = FatFs_dirbase;
    	}
    	scan->index = 0;
    } else { // retrieve
        clust  = scan->clust;
    }

    if(offset != 0) {
        while(scan->index != offset) {
    		if (!move_window(scan->sect))
    		    return NULL;
            if (!next_dir_entry(scan))
                return NULL;
        }
    }

	do {
		if (!move_window(scan->sect)) return NULL;
		dptr = &(FatFs_win[(scan->index & 15) * 32]);		/* Pointer to the directory entry */
		c = *dptr;
		if ((c == 0) || (c == 0xE5)) return dptr;		/* Found an empty entry! */
	} while (next_dir_entry(scan));						/* Next directory pointer */
	/* Reached to end of the directory table */

	/* Abort when static table or could not stretch dynamic table */
	if ((!clust) || !(clust = create_chain(scan->clust))) return NULL;
	if (!move_window(0)) return 0;

	FatFs_winsect = sector = clust2sect(clust);			/* Cleanup the expanded table */
	memset(FatFs_win, 0, 512);
	for (n = FatFs_sects_clust; n; n--) {
		if (disk_write(FatFs_win, sector, 1) != RES_OK) return NULL;
		sector++;
	}
	FatFs_winflag = 1;
	return FatFs_win;
}

/*-----------------------------------------------*/
/* Reserve more than one Directory Entry for LFN */

BYTE* reserve_direntries (
	DIR *scan,			/* Target directory to create new entry */
    BYTE size           /* Number of entries needed, >1 for LFN */
)
{
    BYTE *p;
    WORD idx;    
    WORD offset = 0;
    BYTE cnt = 0;

    while(cnt < size) {
        p = reserve_direntry(scan, offset);
        offset = scan->index + 1;

        if(!p)
            return NULL;
    
        if(cnt == 0) {
            idx  = scan->index;
        } else if(scan->index != (idx+cnt)) { // the next found free entry is not consecutive
            // reset search to this index
            cnt  = 0;
            idx  = scan->index;
        }
        cnt++; // number of free cells found
    }
    // go back to first entry and return its pointer. We know it's free so we can call the same function
    return reserve_direntry(scan, idx);
}    

BOOL valid_char(char c)
{
    if((c >= 'A')&&(c <= 'Z'))
        return 1;
    if((c >= 'a')&&(c <= 'z'))
        return 1;
    if((c >= '0')&&(c <= '9'))
        return 1;
    if((c == '-')||(c == '_'))
        return 1;
    return 0;
}
#endif /* _FS_READONLY */

#ifndef _FS_READONLY

FRESULT create_lfn (
    DIR  *scan,         /* opened directory to create file in */
    char *name,         /* long file name. Note, this routine does not check for valid chars! */
    BYTE **ptr )        /* pointer to dir entry with short file name */
{
    static BYTE lfn[32];
    BYTE len;
    BYTE num;
    BYTE chk;
    BYTE ofs;
    BYTE end;
    BYTE *p;
    BYTE b,c,ext;
    BOOL trunc = 0;
    WORD idx;
    char shortname[12];
    char tilde[8];
    DWORD dw;
        
    /* create short name first */
    memset(shortname, 32, 11); // put spaces
    shortname[11] = 0;
    
    len = strlen(name);

//    printf("Creating short file name out of '%s'.\n", name);

    // get extension
    ext = len;
    b = len-1;
    do {
        if (name[b]=='.') {
            ext = b;
            for(c=0,b++;c<3;b++) {
                if (!name[b])
                    break;
                if(valid_char(name[b])) {
                    shortname[8+c] = name[b];
                    c++;
                } else {
                    shortname[8+c] = '_';
                    c++;
                    trunc = 1;
                }
            }
            break;
        }
        b--;
    } while(b != 0);

//    printf("Short name ext = '%s'. Trunc = %d\n", shortname, trunc);

    if(ext != len) { // there was an extension
        trunc = trunc || ((c==3)&&(b<len));
    }

    // get base name
    for(b=0,c=0;((b<len)&&(b<ext)&&(c<8));b++) {
        if(valid_char(name[b]))
            shortname[c++] = name[b];
        else {
//            shortname[c++] = '_';
            trunc = 1;
        }
    }

    // c is now the first character after the base name.
    
    trunc = ((b<len)&&(b<ext)) || (c<b) || trunc;

//    printf("Short name before capitalize = '%s'. Trunc = %d\n", shortname, trunc);

    // make all caps
    for(b=0;b<11;b++) {
        if((shortname[b] >= 'a')&&(shortname[b] <= 'z'))
            shortname[b] -= 0x20;
    }

    // calculate how many dir entries we need and reserve them
    num = len / 13;
    num += 2;      // 0 => 2, 12 => 2, 13 => 3 (including the terminating zero, and the short name entry)

    p = reserve_direntries(scan, num);
    
    if(!p)
        return FR_DISK_FULL;
        
    // make name unique by slipping index into the filename, but only when we truncated the name)
    if(trunc) {
        idx = scan->index >> 1;  // since an LFN fileentry is at least 2 entries, we can safely divide by 2.
        sprintf(tilde, "~%d", idx);
        b = strlen(tilde);
        if((8-b) <= c)
            c=8-b;
        memcpy(&shortname[c], tilde, b);
    }
    
    chk = calc_lfn_checksum((BYTE *)shortname);

    memset(lfn, 0xff, 32);
    
    // now create each of the entries... starting with the last
    end = 0x40;
    while(--num) {
        ofs = 13*(num-1);
        lfn[0] = num | end;
        end = 0;
        for(b=1;(b<=30)&&(ofs<=len);b+=2,ofs++) {
            lfn[b] = name[ofs];
            lfn[b+1] = 0;
            if(b==9) b+=3; /* jump to 14 */
            if(b==24) b+=2; /* jump to 28 */
        }
        lfn[11] = AM_LFN;
        lfn[12] = 0;
        lfn[13] = chk;
        lfn[26] = 0;
        lfn[27] = 0;
        
        memcpy(p, lfn, 32);  // write back in directory!

        // sync
		FatFs_winflag = 1;

        // get next entry to fill        
        p = reserve_direntry(scan, scan->index + 1);
    }

    // make last entry with the the 8.3 name
    memcpy(p, shortname, 11);
    p[11] = AM_ARC;
    p[12] = 0;
	memset(p+13, 0, 32-13);
	dw = get_fattime();
	ST_DWORD(p+14, dw);	/* Created time */
	ST_DWORD(p+22, dw);	/* Updated time */

    *ptr = p;
	FatFs_winflag = 1;

	return FR_OK;
}

#endif

#ifdef _FS_LFN

FRESULT f_readdir_lfn (
	DIR *scan,			/* Pointer to the directory object */
	FILINFO *finfo,		/* Pointer to file information to return */
	CHAR *longname,     /* Pointer to where the long filename can be stored */
	BYTE maxlen,        /* Maximum length of long file name buffer */
	CHAR *ext	        /* extra storage space for extension */
)
{
	BYTE *entry, c, i, chk;

	finfo->fname[0] = 0;
	if ((disk_status() & STA_NOINIT) || !FatFs_fs_type) return FR_NOT_READY;

	while (scan->sect) {
		if (!move_window(scan->sect)) return FR_RW_ERROR;
		entry = &(FatFs_win[(scan->index & 15) * 32]);		/* pointer to the directory entry */
		c = *entry;
		if (c == 0) return FR_NO_FILE; 					/* Has it reached to end of dir? */
        if((*(entry+11) & AM_LFN)==AM_LFN) {            /* Long file name entry */
            chk = *(entry+13);                          /* store checksum for comparison later */
            c = (c & 0x1F) - 1;                         /* take lower 5 bits of entry index, and subtract one */
            c += (c << 3) + (c << 2);                   /* multiply by 13, c = start offset in buffer */
            for(i=1;(i<=30)&&(c<maxlen);i+=2,c++) {
                longname[c] = entry[i];
                if(i==9) i+=3; /* jump to 14 */
                if(i==24) i+=2; /* jump to 28 */
            }
        }            
    	else if ((c != 0xE5) && (c != '.') && !(*(entry+11) & AM_VOL)) { /* Is it a valid entry? */
			get_fileinfo(finfo, entry);
            ext[0] = *(entry+8);
            ext[1] = *(entry+9);
            ext[2] = *(entry+10);
            c = calc_lfn_checksum(entry); 
			if(c != chk) {
			    longname[0] = '\0'; // kill long file name
			}
	    }

		if (!next_dir_entry(scan)) scan->sect = 0;		/* Next entry */

		if (finfo->fname[0]) {
			return FR_OK;				/* Found valid entry */
		}
	}

	return FR_NO_FILE;
	
}
#endif /* _FS_LFN */

#ifdef _FS_LFN
static
FRESULT f_readdir_scans (
	DIR *scan,			/* Pointer to the directory object */
	DIR *firstscan,		/* Pointer to the directory object that holds the first entry */
	DIR *lastscan)		/* Pointer to the directory object that holds the last entry */
{
	BOOL first, copy, done;
	BYTE *entry, c, chk;

	if ((disk_status() & STA_NOINIT) || !FatFs_fs_type) return FR_NOT_READY;

	first = done = FALSE; // assume we will not read anything
	while (scan->sect) {
		if (!move_window(scan->sect)) return FR_RW_ERROR;
		entry = &(FatFs_win[(scan->index & 15) * 32]);		/* pointer to the directory entry */
		c = *entry;
		if (c == 0) return FR_NO_FILE;					/* Has it reached to end of dir? */
        if((*(entry+11) & AM_LFN)==AM_LFN) {            /* Long file name entry */
            chk = *(entry+13);                          /* store checksum for comparison later */
			copy = TRUE;
        }            
    	else if ((c != 0xE5) && (c != '.') && !(*(entry+11) & AM_VOL)) { /* Is it a valid entry? */
            c = calc_lfn_checksum(entry); 
			if(c != chk) {
			    // Forget the other entries
   				first = FALSE;	
				copy = TRUE;				
			}	
			memcpy(lastscan, scan, sizeof(DIR)); // the caller would like to know	
			done = TRUE;
	    }
		else {
			copy = FALSE; // Its nothing forget it
			first = FALSE;
		}

		if(!first && copy) { // Did we get it?
			memcpy(firstscan, scan, sizeof(DIR)); // the caller would like to know
			first = TRUE;
		}
		
		if (!next_dir_entry(scan)) scan->sect = 0;		/* Next entry */

		if(done) { 
			return FR_OK;
		}
	}

	return FR_NO_FILE;
}

static
FRESULT find_scans_fcluster(
	DIR		*scan,			/* opened directory to find file in */
	DWORD	fcluster,		/* cluster the file starts in */
	DIR 	*firstscan,		/* Pointer to the directory object that holds the first entry */
	DIR 	*lastscan )		/* Pointer to the directory object that holds the last entry */
{
	FRESULT fres;
	DWORD foundcluster;
	BYTE *entry;

	do {
		fres = f_readdir_scans(scan, firstscan, lastscan);
		entry = &(FatFs_win[(lastscan->index & 15) * 32]);		/* pointer to the directory entry */
	    foundcluster = ((DWORD)LD_WORD(entry+20) << 16) | LD_WORD(entry+26); /* Cluster */
		} while((fres == FR_OK) && (fcluster != foundcluster));
	return fres;
}

FRESULT find_lfn(
	DIR  *scan,         /* opened directory to find file in */
    char *name,         /* long file name. Note, this routine does not check for valid chars! */
	DIR  *firstscan,	/* Pointer to the directory object that holds the first entry */
	DIR  *lastscan)		/* Pointer to the directory object that holds the last entry */
{
	static FILINFO finfo;
	static FRESULT fres;
	static char casedname[MAX_ENTRY_NAME+1];
    static char ext[4];
	WORD entries = 0;
	BYTE len, i;
	
	memcpy(&tempscan, scan, sizeof(tempscan)); // Keep this as we first want to find the bloody file

	// I DONT HAVE A STRICMP CAUSE I AM A PAUPER
	len = data_strnlen(name, MAX_ENTRY_NAME);
	for(i = 0; i < len; i++) {
		casedname[i] = name[i] | 0x20;
	}
    casedname[i] = 0;
    
//    printf("find lfn '%s' Cased: '%s'\n", name, casedname);
	
    ext[3] = 0;
	do {
		memset(&temp_name, 0, NR_OF_EL(temp_name));

		fres = f_readdir_lfn(&tempscan, &finfo, temp_name, NR_OF_EL(temp_name), ext);

		if(fres == FR_OK) {
            if(!(temp_name[0]) && finfo.fname[0]) { // if the file has no long file name
                strncpy(temp_name, &finfo.fname[0], NR_OF_EL(temp_name)); // use shorty for it. who cares?
            }

			// I DONT HAVE A STRICMP CAUSE I AM A PAUPER
			len = data_strnlen(temp_name, NR_OF_EL(temp_name));
			for(i = 0; i < len; i++) {
				temp_name[i] |= 0x20;
			}

//            printf("temp name = '%s'\n", temp_name);
            
			if(strncmp(temp_name, casedname, NR_OF_EL(temp_name)) == 0) { // compare the file name to what we want.
				// This is her ok. now get the scans for real this time because the other function forgot :S
				memcpy(&tempscan, scan, sizeof(tempscan)); // Keep this as we dont want to change the callers scan
				fres = find_scans_fcluster(&tempscan, finfo.fcluster, firstscan, lastscan);
				return fres;
			}
		}
	} while(fres == FR_OK);	

// This might be overdoing it.
	memset(firstscan, 0, sizeof(DIR));
	memset(lastscan, 0, sizeof(DIR));

	return FR_NO_FILE;
}
#endif /* _FS_LFN */

/*-----------------------------------------*/
/* Make Sure that the File System is Valid */

FRESULT check_mounted ()
{

	if (disk_status() & STA_NOINIT) {	/* The drive has not been initialized */
		if (FatFs_files)					/* Drive was uninitialized with any file left opend */
			return FR_INCORRECT_DISK_CHANGE;
		else
			return f_mountdrv();		/* Initialize file system and return resulut */
	} else {							/* The drive has been initialized */
		if (!FatFs_fs_type)				/* But the file system has not been initialized */
			return f_mountdrv();		/* Initialize file system and return resulut */
	}
	return FR_OK;						/* File system is valid */
}


/*--------------------------------------------------------------------------*/
/* Public Funciotns                                                         */
/*--------------------------------------------------------------------------*/


/*----------------------------------------------------------*/
/* Load File System Information and Initialize FatFs Module */

FRESULT f_mountdrv (void)
{
	BYTE fat;
	DWORD sect, fatend, maxsect;

	/* Initialize file system object */
//	memset(&FatFs, 0, sizeof(FATFS));
	memset(&FatFs_fs_type, 0, 35);
	memset(&FatFs_win, 0, 512);

	/* Initialize disk drive */
	if (disk_initialize() & STA_NOINIT)	return FR_NOT_READY;

	/* Search FAT partition */
	fat = check_fs(sect = 0);		/* Check sector 0 as an SFD format */
	if (!fat) {						/* Not a FAT boot record, it will be an FDISK format */
		/* Check a partition listed in top of the partition table */
		if (FatFs_win[0x1C2]) {					/* Is the partition existing? */
			sect = LD_DWORD(&(FatFs_win[0x1C6]));	/* Partition offset in LBA */
			fat = check_fs(sect);				/* Check the partition */
		}
	}
	if (!fat) return FR_NO_FILESYSTEM;	/* No FAT patition */

	/* Initialize file system object */
	FatFs_fs_type = fat;								/* FAT type */
	FatFs_sects_fat = 								/* Sectors per FAT */
		(fat == FS_FAT32) ? LD_DWORD(&(FatFs_win[0x24])) : LD_WORD(&(FatFs_win[0x16]));
	FatFs_sects_clust = FatFs_win[0x0D];				/* Sectors per cluster */
	FatFs_n_fats = FatFs_win[0x10];						/* Number of FAT copies */
	FatFs_fatbase = sect + LD_WORD(&(FatFs_win[0x0E]));	/* FAT start sector (physical) */
	FatFs_n_rootdir = LD_WORD(&(FatFs_win[0x11]));		/* Nmuber of root directory entries */

	fatend = FatFs_sects_fat * FatFs_n_fats + FatFs_fatbase;
	if (fat == FS_FAT32) {
		FatFs_dirbase = LD_DWORD(&(FatFs_win[0x2C]));	/* FAT32: Directory start cluster */
		FatFs_database = fatend;	 					/* FAT32: Data start sector (physical) */
	} else {
		FatFs_dirbase = fatend;						/* Directory start sector (physical) */
		FatFs_database = FatFs_n_rootdir / 16 + fatend;	/* Data start sector (physical) */
	}
	maxsect = LD_DWORD(&(FatFs_win[0x20]));			/* Calculate maximum cluster number */
	if (!maxsect) maxsect = LD_WORD(&(FatFs_win[0x13]));
	FatFs_max_clust = (maxsect - FatFs_database + sect) / FatFs_sects_clust + 2;

	return FR_OK;
}



/*-----------------------*/
/* Open or Create a File */

FRESULT f_open (
	FIL *fp,			/* Pointer to the buffer of new file object to create */
	const CHAR *path,	/* Pointer to the file name */
	BYTE mode			/* Access mode and file open mode flags */
)
{
	FRESULT res;
	BYTE *dir;
	DIR dirscan;
	CHAR fn[8+3+1];

	if ((res = check_mounted()) != FR_OK) return res;
#ifndef _FS_READONLY
	if ((mode & (FA_WRITE|FA_CREATE_ALWAYS|FA_OPEN_ALWAYS)) && (disk_status() & STA_PROTECT))
		return FR_WRITE_PROTECTED;
#endif

	res = trace_path(&dirscan, fn, path, &dir);	/* Trace the file path */

#ifndef _FS_READONLY
	/* Create or Open a File */
	if (mode & (FA_CREATE_ALWAYS|FA_OPEN_ALWAYS)) {
		DWORD dw;
		if (res != FR_OK) {		/* No file, create new */
			mode |= FA_CREATE_ALWAYS;
			if (res != FR_NO_FILE) return res;
			dir = reserve_direntry(&dirscan, 0);	/* Reserve a directory entry */
			if (dir == NULL) return FR_DENIED;
			memcpy(dir, fn, 8+3);		/* Initialize the new entry */
			*(dir+12) = fn[11];
			memset(dir+13, 0, 32-13);
		} else {				/* Any object is already existing */
			if ((dir == NULL) || (*(dir+11) & (AM_RDO|AM_DIR)))	/* Could not overwrite (R/O or DIR) */
				return FR_DENIED;
			if (mode & FA_CREATE_ALWAYS) {	/* Resize it to zero */
				dw = FatFs_winsect;			/* Remove the cluster chain */
				if (!remove_chain(((DWORD)LD_WORD(dir+20) << 16) | LD_WORD(dir+26))
					|| !move_window(dw) )
					return FR_RW_ERROR;
				ST_WORD(dir+20, 0); ST_WORD(dir+26, 0);	/* cluster = 0 */
				ST_DWORD(dir+28, 0);					/* size = 0 */
			}
		}
		if (mode & FA_CREATE_ALWAYS) {
            if(mode & FA_CREATE_HIDDEN) {
			    *(dir+11) = AM_SYS | AM_HID;
			} else {
			    *(dir+11) = AM_ARC;
            }
			dw = get_fattime();
			ST_DWORD(dir+14, dw);	/* Created time */
			ST_DWORD(dir+22, dw);	/* Updated time */
			FatFs_winflag = 1;
		}
	}
	/* Open a File */
	else {
#endif /* _FS_READONLY */
		if (res != FR_OK) return res;		/* Trace failed */
		if ((dir == NULL) || (*(dir+11) & AM_DIR))	/* It is a directory */
			return FR_NO_FILE;
#ifndef _FS_READONLY
		if ((mode & FA_WRITE) && (*(dir+11) & AM_RDO)) /* R/O violation */
			return FR_DENIED;
	}
#endif

#ifdef _FS_READONLY
	fp->flag = mode & FA_READ;
#else
	fp->flag = mode & (FA_WRITE|FA_READ);
	fp->dir_sect = FatFs_winsect;			/* Pointer to the directory entry */
	fp->dir_ptr  = dir;
#endif
	fp->org_clust =	((DWORD)LD_WORD(dir+20) << 16) | LD_WORD(dir+26);	/* File start cluster */
	fp->fsize = LD_DWORD(dir+28);		/* File size */
	fp->fptr = 0;						/* File ptr */
	fp->sect_clust = 1;					/* Sector counter */
	FatFs_files++;
	return FR_OK;
}

FRESULT f_open_direct (
	FIL *fp,			/* Pointer to the buffer of new file object to create */
	DIRENTRY *entry
	)
{
	FRESULT res;

	if ((res = check_mounted()) != FR_OK) return res;

#ifdef _FS_READONLY
	fp->flag = FA_READ;         /* we might need to change this */
//	fp->dir_sect = 0L;			/* Pointer to the directory entry */
//	fp->dir_ptr  = NULL;         /* set to NULL, to indicate that we never want to change it */
#else
	fp->flag     = FA_READ | ((entry->fattrib & AM_RDO) ? 0 : FA_WRITE); 
	fp->dir_sect = 0L;			/* Pointer to the directory entry */
	fp->dir_ptr  = NULL;         /* set to NULL, to indicate that we never want to change it */
//	fp->dir_sect = FatFs_winsect;			/* Pointer to the directory entry */
//	fp->dir_ptr  = dir;
#endif
	fp->org_clust =	entry->fcluster;
	fp->fsize = entry->fsize;
	fp->fptr = 0;				/* File ptr */
	fp->sect_clust = 1;			/* Sector counter */

	FatFs_files++;
	return FR_OK;
}

/*-----------*/
/* Read File */

FRESULT f_read (
	FIL *fp, 		/* Pointer to the file object */
	void *buff,		/* Pointer to data buffer */
	WORD btr,		/* Number of bytes to read */
	WORD *br		/* Pointer to number of bytes read */
)
{
	DWORD clust, sect, ln;
	WORD rcnt;
	BYTE cc, *rbuff = buff;

//    printf("f_read(%p,%p,$%x,%p);\n", fp, buff, btr, br);

	*br = 0;
	if ((disk_status() & STA_NOINIT) || !FatFs_fs_type) return FR_NOT_READY;	/* Check disk ready */
	if (fp->flag & FA__ERROR) return FR_RW_ERROR;	/* Check error flag */
	if (!(fp->flag & FA_READ)) return FR_DENIED;	/* Check access mode */
	ln = fp->fsize - fp->fptr;
	if (btr > ln) btr = (WORD)ln;					/* Truncate read count by number of bytes left */

	for ( ;  btr;									/* Repeat until all data transferred */
		rbuff += rcnt, fp->fptr += rcnt, *br += rcnt, btr -= rcnt) {
		if ((fp->fptr % 512) == 0) {				/* On the sector boundary */
			if (--(fp->sect_clust)) {				/* Decrement left sector counter */
				sect = fp->curr_sect + 1;			/* Get current sector */
			} else {								/* On the cluster boundary, get next cluster */
				clust = (fp->fptr == 0) ? fp->org_clust : get_cluster(fp->curr_clust);
//                printf(".rd.%ld.%ld.", fp->curr_clust, clust);    
				if ((clust < 2) || (clust >= FatFs_max_clust)) goto fr_error;
				fp->curr_clust = clust;				/* Current cluster */
				sect = clust2sect(clust);			/* Get current sector */
				fp->sect_clust = FatFs_sects_clust;	/* Re-initialize the left sector counter */
			}
#ifndef _FS_READONLY
			if (fp->flag & FA__DIRTY) {				/* Flush file I/O buffer if needed */
				if (disk_write(fp->buffer, fp->curr_sect, 1) != RES_OK) goto fr_error;
				fp->flag &= ~FA__DIRTY;
			}
#endif
			fp->curr_sect = sect;					/* Update current sector */
			cc = btr / 512;							/* When left bytes >= 512, */
			if (cc) {								/* Read maximum contiguous sectors directly */
				if (cc > fp->sect_clust) cc = fp->sect_clust;
				if (disk_read(rbuff, sect, cc) != RES_OK) goto fr_error;
				fp->sect_clust -= cc - 1;
				fp->curr_sect += cc - 1;
				rcnt = cc * 512; continue;
			}
			if (disk_read(fp->buffer, sect, 1) != RES_OK)	/* Load the sector into file I/O buffer */
				goto fr_error;
		}
		rcnt = 512 - ((WORD)fp->fptr % 512);				/* Copy fractional bytes from file I/O buffer */
		if (rcnt > btr) rcnt = btr;
		memcpy(rbuff, &fp->buffer[fp->fptr % 512], rcnt);
	}

	return FR_OK;

fr_error:	/* Abort this file due to an unrecoverable error */
	fp->flag |= FA__ERROR;
	return FR_RW_ERROR;
}



/*------------*/
/* Write File */

#ifndef _FS_READONLY
FRESULT f_write (
	FIL *fp,			/* Pointer to the file object */
	const void *buff,	/* Pointer to the data to be written */
	WORD btw,			/* Number of bytes to write */
	WORD *bw			/* Pointer to number of bytes written */
)
{
	DWORD clust, sect;
	WORD wcnt;
	BYTE cc;
	const BYTE *wbuff = buff;

	*bw = 0;
	if ((disk_status() & STA_NOINIT) || !FatFs_fs_type) return FR_NOT_READY;
	if (fp->flag & FA__ERROR) return FR_RW_ERROR;	/* Check error flag */
	if (!(fp->flag & FA_WRITE)) return FR_DENIED;	/* Check access mode */
	if (fp->fsize + btw < fp->fsize) btw = 0;		/* File size cannot reach 4GB */

	for ( ;  btw;									/* Repeat until all data transferred */
		wbuff += wcnt, fp->fptr += wcnt, *bw += wcnt, btw -= wcnt) {
		if ((fp->fptr % 512) == 0) {				/* On the sector boundary */
			if (--(fp->sect_clust)) {				/* Decrement left sector counter */
				sect = fp->curr_sect + 1;			/* Get current sector */
			} else {								/* On the cluster boundary, get next cluster */
				if (fp->fptr == 0) {				/* Is top of the file */
					clust = fp->org_clust;
					if (clust == 0)					/* No cluster is created yet */
						fp->org_clust = clust = create_chain(0);	/* Create a new cluster chain */
				} else {							/* Middle or end of file */
					clust = create_chain(fp->curr_clust);			/* Trace or streach cluster chain */
				}
				if ((clust < 2) || (clust >= FatFs_max_clust)) break;
				fp->curr_clust = clust;				/* Current cluster */
				sect = clust2sect(clust);			/* Get current sector */
				fp->sect_clust = FatFs_sects_clust;	/* Re-initialize the left sector counter */
			}
			if (fp->flag & FA__DIRTY) {				/* Flush file I/O buffer if needed */
				if (disk_write(fp->buffer, fp->curr_sect, 1) != RES_OK) goto fw_error;
				fp->flag &= ~FA__DIRTY;
			}
			fp->curr_sect = sect;					/* Update current sector */
			cc = btw / 512;							/* When left bytes >= 512, */
			if (cc) {								/* Write maximum contiguous sectors directly */
				if (cc > fp->sect_clust) cc = fp->sect_clust;
				if (disk_write(wbuff, sect, cc) != RES_OK) goto fw_error;
				fp->sect_clust -= cc - 1;
				fp->curr_sect += cc - 1;
				wcnt = cc * 512; continue;
			}
			if ((fp->fptr < fp->fsize) &&  			/* Fill sector buffer with file data if needed */
				(disk_read(fp->buffer, sect, 1) != RES_OK))
					goto fw_error;
		}
		wcnt = 512 - ((WORD)fp->fptr % 512);		/* Copy fractional bytes to file I/O buffer */
		if (wcnt > btw) wcnt = btw;
		memcpy(&fp->buffer[fp->fptr % 512], wbuff, wcnt);
		fp->flag |= FA__DIRTY;
	}

	if (fp->fptr > fp->fsize) fp->fsize = fp->fptr;	/* Update file size if needed */
	fp->flag |= FA__WRITTEN;						/* Set file changed flag */
	return FR_OK;

fw_error:	/* Abort this file due to an unrecoverable error */
	fp->flag |= FA__ERROR;
	return FR_RW_ERROR;
}
#endif /* _FS_READONLY */



/*-------------------*/
/* Seek File Pointer */

FRESULT f_lseek (
	FIL *fp,		/* Pointer to the file object */
	DWORD ofs		/* File pointer from top of file */
)
{
	DWORD clust;
	BYTE sc;

//    printf("[seek]");

	if ((disk_status() & STA_NOINIT) || !FatFs_fs_type) return FR_NOT_READY;
	if (fp->flag & FA__ERROR) return FR_RW_ERROR;
#ifndef _FS_READONLY
	if (fp->flag & FA__DIRTY) {			/* Write-back dirty buffer if needed */
		if (disk_write(fp->buffer, fp->curr_sect, 1) != RES_OK) goto fk_error;
		fp->flag &= ~FA__DIRTY;
	}
#endif
	if (ofs > fp->fsize) ofs = fp->fsize;	/* Clip offset by file size */
	fp->fptr = ofs; fp->sect_clust = 1; 	/* Re-initialize file pointer */

	/* Seek file pinter if needed */
	if (ofs) {
		ofs = (ofs - 1) / 512;				/* Calcurate current sector */
		sc = FatFs_sects_clust;				/* Number of sectors in a cluster */
		fp->sect_clust = sc - ((BYTE)ofs % sc);	/* Calcurate sector counter */
		ofs /= sc;							/* Number of clusters to skip */
		clust = fp->org_clust;				/* Seek to current cluster */
		while (ofs--)
			clust = get_cluster(clust);
		if ((clust < 2) || (clust >= FatFs_max_clust)) goto fk_error;
		fp->curr_clust = clust;
		fp->curr_sect = clust2sect(clust) + sc - fp->sect_clust;	/* Current sector */
		if (fp->fptr % 512) {										/* Load currnet sector if needed */
			if (disk_read(fp->buffer, fp->curr_sect, 1) != RES_OK)
				goto fk_error;
		}
	}

	return FR_OK;

fk_error:	/* Abort this file due to an unrecoverable error */
	fp->flag |= FA__ERROR;
	return FR_RW_ERROR;
}



/*-------------------------------------------------*/
/* Synchronize between File and Disk without Close */

#ifndef _FS_READONLY
FRESULT f_sync (
	FIL *fp		/* Pointer to the file object */
)
{
	BYTE *ptr;

	if ((disk_status() & STA_NOINIT) || !FatFs_fs_type)
		return FR_INCORRECT_DISK_CHANGE;

	/* Has the file been written? */
	if (fp->flag & FA__WRITTEN) {
		/* Write back data buffer if needed */
		if (fp->flag & FA__DIRTY) {
			if (disk_write(fp->buffer, fp->curr_sect, 1) != RES_OK) return FR_RW_ERROR;
			fp->flag &= ~FA__DIRTY;
		}
        if(fp->dir_ptr) { /* should we update the directory? not for some files */
//            printf("Sync update dir. (Sect = %ld.Fsize=%ld)\n", fp->dir_sect, fp->fsize);
    		/* Update the directory entry */
    		if (!move_window(fp->dir_sect)) return FR_RW_ERROR;
    		ptr = fp->dir_ptr;
    		*(ptr+11) |= AM_ARC;					/* Set archive bit */
    		ST_DWORD(ptr+28, fp->fsize);			/* Update file size */
    		ST_WORD(ptr+26, fp->org_clust);			/* Update start cluster */
    		ST_WORD(ptr+20, fp->org_clust >> 16);
    		ST_DWORD(ptr+22, get_fattime());		/* Updated time */
    		FatFs_winflag = 1;
    		fp->flag &= ~FA__WRITTEN;
        	if (!move_window(0)) return FR_RW_ERROR;
        }
	}

	return FR_OK;
}
#endif /* _FS_READONLY */



/*------------*/
/* Close File */

FRESULT f_close (
	FIL *fp		/* Pointer to the file object to be closed */
)
{
	FRESULT res;


#ifndef _FS_READONLY
	res = f_sync(fp);
#else
	res = FR_OK;
#endif
	if (res == FR_OK) {
		fp->flag = 0;
        fp->org_clust = 0L; // clear, so if we want to know if the file is open, we can check this field.
		FatFs_files--;
	}
	return res;
}



#if _FS_MINIMIZE <= 1
/*---------------------------*/
/* Initialize directroy scan */

FRESULT f_opendir (
	DIR *scan,			/* Pointer to directory object to initialize */
	const CHAR *path	/* Pointer to the directory path, null str means the root */
)
{
	FRESULT res;
	BYTE *dir;
	CHAR fn[8+3+1];


	if ((res = check_mounted()) != FR_OK) return res;

	res = trace_path(scan, fn, path, &dir);	/* Trace the directory path */

	if (res == FR_OK) {						/* Trace completed */
		if (dir != NULL) {					/* It is not a root dir */
			if (*(dir+11) & AM_DIR) {		/* The entry is a directory */
				scan->clust = ((DWORD)LD_WORD(dir+20) << 16) | LD_WORD(dir+26);
				scan->sect = clust2sect(scan->clust);
				scan->index = 0;
			} else {						/* The entry is not directory */
				res = FR_NO_FILE;
			}
		}
	}
	return res;
}

FRESULT f_opendir_direct (
	DIR *scan,			/* Pointer to directory object to initialize */
	const DWORD cluster	/* First cluster of directory, zero means the root */
)
{
	FRESULT res;

	if ((res = check_mounted()) != FR_OK) return res;

    if(!cluster) {
    	if (FatFs_fs_type == FS_FAT32) {
    		scan->clust = scan->sclust = FatFs_dirbase;
    		scan->sect = clust2sect(FatFs_dirbase);
    	} else {
    		scan->clust = scan->sclust = 0;
    		scan->sect = FatFs_dirbase;
    	}
    } else {
    	scan->clust = cluster;
    	scan->sect = clust2sect(scan->clust);
    }
	scan->index = 0;
    scan->sclust = scan->clust;
    
	return FR_OK;
}

/*----------------------------------*/
/* Read Directory Entry in Sequense */

FRESULT f_readdir (
	DIR *scan,			/* Pointer to the directory object */
	FILINFO *finfo		/* Pointer to file information to return */
)
{
	BYTE *dir, c;

	finfo->fname[0] = 0;
	if ((disk_status() & STA_NOINIT) || !FatFs_fs_type) return FR_NOT_READY;

	while (scan->sect) {
		if (!move_window(scan->sect)) return FR_RW_ERROR;
		dir = &(FatFs_win[(scan->index & 15) * 32]);		/* pointer to the directory entry */
		c = *dir;
		if (c == 0) return FR_NO_FILE;								/* Has it reached to end of dir? */
		if ((c != 0xE5) && (c != '.') && !(*(dir+11) & AM_VOL))	/* Is it a valid entry? */
			get_fileinfo(finfo, dir);
		if (!next_dir_entry(scan)) scan->sect = 0;		/* Next entry */
		if (finfo->fname[0]) return FR_OK;						/* Found valid entry */
	}

	return FR_NO_FILE;
}
#endif /* _FS_MINIMIZE <= 1 */

#if _FS_MINIMIZE == 0
/*-----------------*/
/* Get File Status */

FRESULT f_stat (
	const CHAR *path,	/* Pointer to the file path */
	FILINFO *finfo		/* Pointer to file information to return */
)
{
	FRESULT res;
	BYTE *dir;
	DIR dirscan;
	CHAR fn[8+3+1];


	if ((res = check_mounted()) != FR_OK) return res;

	res = trace_path(&dirscan, fn, path, &dir);	/* Trace the file path */

	if (res == FR_OK)							/* Trace completed */
		get_fileinfo(finfo, dir);

	return res;
}
#endif /*  _FS_MINIMIZE == 0 */

#ifndef _FS_READONLY
/*-----------------------------*/
/* Get Number of Free Clusters */

FRESULT f_getfree (
	DWORD *nclust		/* Pointer to the double word to return number of free clusters */
)
{
	DWORD n, clust, sect;
	BYTE fat, f, *p;
	FRESULT res;

	if ((res = check_mounted()) != FR_OK) return res;

	/* Count number of free clusters */
	fat = FatFs_fs_type;
	n = 0;
#ifdef _FS_FAT12
	if (fat == FS_FAT12) {
		clust = 2;
		do {
			if ((WORD)get_cluster(clust) == 0) n++;
		} while (++clust < FatFs_max_clust);
	} else {
#endif
		clust = FatFs_max_clust;
		sect = FatFs_fatbase;
		f = 0; p = 0;
		do {
			if (!f) {
				if (!move_window(sect++)) return FR_RW_ERROR;
				p = FatFs_win;
			}
			if (fat == FS_FAT16) {
				if (LD_WORD(p) == 0) n++;
				p += 2; f += 1;
			} else {
				if (LD_DWORD(p) == 0) n++;
				p += 4; f += 2;
			}
		} while (--clust);
#ifdef _FS_FAT12
	}
#endif
	*nclust = n;
	return FR_OK;
}

#endif

#ifndef _FS_READONLY

/*------------------------------*/
/* Delete a File or a Directory */

FRESULT f_unlink (
	const CHAR *path			/* Pointer to the file or directory path */
)
{
	FRESULT res;
	BYTE *dir, *sdir;
	DWORD dclust, dsect;
	DIR dirscan;
	CHAR fn[8+3+1];

	if ((res = check_mounted()) != FR_OK) return res;
	if (disk_status() & STA_PROTECT) return FR_WRITE_PROTECTED;

	res = trace_path(&dirscan, fn, path, &dir);	/* Trace the file path */

	if (res != FR_OK) return res;				/* Trace failed */
	if (dir == NULL) return FR_NO_FILE;			/* It is a root directory */
	if (*(dir+11) & AM_RDO) return FR_DENIED;	/* It is a R/O item */
	dsect = FatFs_winsect;
	dclust = ((DWORD)LD_WORD(dir+20) << 16) | LD_WORD(dir+26);

	if (*(dir+11) & AM_DIR) {					/* It is a sub-directory */
		dirscan.clust = dclust;					/* Check if the sub-dir is empty or not */
		dirscan.sect = clust2sect(dclust);
		dirscan.index = 0;
		do {
			if (!move_window(dirscan.sect)) return FR_RW_ERROR;
			sdir = &(FatFs_win[(dirscan.index & 15) * 32]);
			if (*sdir == 0) break;
			if (!((*sdir == 0xE5) || (*sdir == '.')) && !(*(sdir+11) & AM_VOL))
				return FR_DENIED;	/* The directory is not empty */
		} while (next_dir_entry(&dirscan));
	}

	if (!move_window(dsect)) return FR_RW_ERROR;	/* Mark the directory entry 'deleted' */
	*dir = 0xE5; 
	FatFs_winflag = 1;
	if (!remove_chain(dclust)) return FR_RW_ERROR;	/* Remove the cluster chain */
	if (!move_window(0)) return FR_RW_ERROR;

	return FR_OK;
}



/*--------------------*/
/* Create a Directory */

FRESULT init_dir (
    DWORD pclust,         /* cluster number of parent directory */
    DWORD *dclust )       /* pointer to cluster number where new directory was created */
{
	DWORD sect, dsect, tim;
    BYTE *w, n;
    
	sect = FatFs_winsect;
	dsect = clust2sect(*dclust = create_chain(0));	/* Get a new cluster for new directory */
	if (!dsect) return FR_DENIED;
	if (!move_window(0)) return FR_RW_ERROR;

	w = FatFs_win;
	memset(w, 0, 512);						/* Initialize the directory table */
	for (n = FatFs_sects_clust - 1; n; n--) {
		if (disk_write(w, dsect+n, 1) != RES_OK) return FR_RW_ERROR;
	}

	FatFs_winsect = dsect;					/* Create dot directories */
	memset(w, ' ', 8+3);
	*w = '.';
	*(w+11) = AM_DIR;
	tim = get_fattime();
	ST_DWORD(w+22, tim);
	ST_WORD(w+26, *dclust);
	ST_WORD(w+20, (*dclust) >> 16);
	memcpy(w+32, w, 32); *(w+33) = '.';

	if (FatFs_fs_type == FS_FAT32 && pclust == FatFs_dirbase)
	    pclust = 0;
	ST_WORD(w+32+26, pclust);
	ST_WORD(w+32+20, pclust >> 16);
	FatFs_winflag = 1;

	if (!move_window(sect)) return FR_RW_ERROR;

    return FR_OK;
}    

/* User function */

FRESULT f_mkdir (
	const CHAR *path		/* Pointer to the directory path */
)
{
	FRESULT res;
	BYTE *dir;
	DIR dirscan;
	DWORD dclust, tim;
	CHAR fn[8+3+1];

	if ((res = check_mounted()) != FR_OK) return res;
	if (disk_status() & STA_PROTECT) return FR_WRITE_PROTECTED;

	res = trace_path(&dirscan, fn, path, &dir);	/* Trace the file path */

	if (res == FR_OK) return FR_DENIED;		/* Any file or directory is already existing */
	if (res != FR_NO_FILE) return res;

	dir = reserve_direntry(&dirscan, 0);		/* Reserve a directory entry */
	if (dir == NULL) return FR_DENIED;

    res = init_dir(dirscan.sclust, &dclust);
    if (res != FR_OK)
        return res;
        
	tim = get_fattime();
	memcpy(dir, fn, 8+3);			/* Initialize the new entry */
	*(dir+11) = AM_DIR;
	*(dir+12) = fn[11];
	memset(dir+13, 0, 32-13);
	ST_DWORD(dir+22, tim);			/* Crated time */
	ST_WORD(dir+26, dclust);		/* Table start cluster */
	ST_WORD(dir+20, dclust >> 16);
	FatFs_winflag = 1;

	if (!move_window(0)) return FR_RW_ERROR;

	return FR_OK;
}

/*-----------------------*/
/* Change File Attribute */

FRESULT f_chmod (
	const CHAR *path,	/* Pointer to the file path */
	BYTE value,			/* Attribute bits */
	BYTE mask			/* Attribute mask to change */
)
{
	FRESULT res;
	BYTE *dir;
	DIR dirscan;
	CHAR fn[8+3+1];

	if ((res = check_mounted()) != FR_OK) return res;
	if (disk_status() & STA_PROTECT) return FR_WRITE_PROTECTED;

	res = trace_path(&dirscan, fn, path, &dir);	/* Trace the file path */

	if (res == FR_OK) {			/* Trace completed */
		if (dir == NULL) {
			res = FR_NO_FILE;
		} else {
			mask &= AM_RDO|AM_HID|AM_SYS|AM_ARC;	/* Valid attribute mask */
			*(dir+11) = (value & mask) | (*(dir+11) & ~mask);	/* Apply attribute change */
			FatFs_winflag = 1;
			if (!move_window(0)) res = FR_RW_ERROR;
		}
	}
	return res;
}



/*-----------------------*/
/* Rename File/Directory */

FRESULT f_rename (
	const CHAR *path_old,	/* Pointer to the old name */
	const CHAR *path_new	/* Pointer to the new name */
)
{
	FRESULT res;
	DWORD sect_old;
	BYTE *dir_old, *dir_new, direntry[32-11];
	DIR dirscan;
	CHAR fn[8+3+1];

	if ((res = check_mounted()) != FR_OK) return res;
	if (disk_status() & STA_PROTECT) return FR_WRITE_PROTECTED;

	res = trace_path(&dirscan, fn, path_old, &dir_old);	/* Check old object */
	if (res != FR_OK) return res;			/* The old object is not found */
	if (!dir_old) return FR_NO_FILE;
	sect_old = FatFs_winsect;					/* Save the object information */
	memcpy(direntry, dir_old+11, 32-11);

	res = trace_path(&dirscan, fn, path_new, &dir_new);	/* Check new object */
	if (res == FR_OK) return FR_DENIED;		/* The new object name is already existing */
	if (res != FR_NO_FILE) return res;

	dir_new = reserve_direntry(&dirscan, 0);	/* Reserve a directory entry */
	if (dir_new == NULL) return FR_DENIED;
	memcpy(dir_new+11, direntry, 32-11);	/* Create new entry */
	memcpy(dir_new, fn, 8+3);
	*(dir_new+12) = fn[11];
	FatFs_winflag = 1;

	if (!move_window(sect_old)) return FR_RW_ERROR;	/* Remove old entry */
	*dir_old = 0xE5;
	FatFs_winflag = 1;
	if (!move_window(0)) return FR_RW_ERROR;

	return FR_OK;
}

static
BOOL f_check_dir_empty (DIR *lastscan)
{
    BYTE   *dir_entry;
	DWORD 	dclust;

	// We can do this because the window is at the lastscan
	dir_entry = &(FatFs_win[(lastscan->index & 15) * 32]); /* pointer to the last directory entry */
	dclust = ((DWORD)LD_WORD(dir_entry+20) << 16) | LD_WORD(dir_entry+26); // start cluster of the file/dir we're looking at

	if (*(dir_entry+11) & AM_DIR) {					/* It is a sub-directory */
		tempscan.clust = dclust;					/* Check if the sub-dir is empty or not */
		tempscan.sect = clust2sect(dclust);
		tempscan.index = 0;
		do {
			if (!move_window(tempscan.sect)) return FR_RW_ERROR;
			dir_entry = &(FatFs_win[(tempscan.index & 15) * 32]);
			if (*dir_entry == 0) break;
			if (!((*dir_entry == 0xE5) || (*dir_entry == '.')) && !(*(dir_entry+11) & AM_VOL)) {
				printf("dir not empty\n");
				return FALSE;	/* The directory is not empty */
			}
		} while (next_dir_entry(&tempscan));
	}
    return TRUE;
}

static
FRESULT f_remove_lfn (
		DIR *firstscan, 
		DIR *lastscan,
		DWORD *clust,
		BOOL check_dir_empty
) 
{
	BYTE 	*dir_entry, c;
    DWORD    dclust;
    	
	// We can do this because the window is at the lastscan
	dir_entry = &(FatFs_win[(lastscan->index & 15) * 32]); /* pointer to the last directory entry */
	dclust = ((DWORD)LD_WORD(dir_entry+20) << 16) | LD_WORD(dir_entry+26); // start cluster of the file/dir we're looking at

	if (*(dir_entry+11) & AM_RDO) { //  This entry tell us the attributes
		return FR_DENIED;	/* It is a R/O item */
	}

    if(check_dir_empty) 
        if(!f_check_dir_empty(lastscan))
            return FR_DENIED;

	// Not so strictly checking on end of directory here. We know the entries are there!
	while (firstscan->sect) {
		if (!move_window(firstscan->sect)) return FR_RW_ERROR;			
		dir_entry = &(FatFs_win[(firstscan->index & 15) * 32]);
		c = *dir_entry;
		if (c == 0) break;	/* Has it reached to end of dir? */
		FatFs_winflag = 1;			
		*dir_entry = 0xE5;	/* clear entry */
		
		if (memcmp(firstscan, lastscan, sizeof(DIR)) == 0) break;	/* Found finished the last entry */
		if (!next_dir_entry(firstscan)) break; // false means end of root!!! (GZW) firstscan->sect = 0; /* Next entry */  
	}
	
	if (!move_window(0)) return FR_RW_ERROR;	/* Writeback: Mark the directory entry 'deleted' */

    *clust = dclust;
    return FR_OK;    
}

/*

    // deallocate
	if (!remove_chain(dclust)) return FR_RW_ERROR;	// Remove the cluster chain

    // purge
	if (!move_window(0)) return FR_RW_ERROR;

	return FR_OK;

}
*/
FRESULT f_unlink_entry (
		DIR *scan,         		/* opened directory to rename entry in */
		char *name				/* entry name to unlink */
)
{
	FRESULT fres;
    DWORD dclust;

	if ((fres = check_mounted()) != FR_OK) return fres;
	if (disk_status() & STA_PROTECT) return FR_WRITE_PROTECTED;

	fres = find_lfn(scan, name, &firstscan, &lastscan); // the first parameter will be modified and thus be the last scan.

	if(fres == FR_OK) {
		fres = f_remove_lfn(&firstscan, &lastscan, &dclust, TRUE);
		if(fres == FR_OK) {
	        if (!remove_chain(dclust))
	            return FR_RW_ERROR;	// Remove the cluster chain
		}
	}

    // purge
	if (!move_window(0))
	    return FR_RW_ERROR;

	return fres;
}

FRESULT f_unlink_direct (
		DIR	*scan,
		DIRENTRY *entry
)
{
	FRESULT fres;
    DWORD dclust;
	
	if(entry->fattrib  & AM_RDO) {
		return FR_DENIED;	/* It is a R/O item */
	}
	fres = find_scans_fcluster(scan, entry->fcluster, &firstscan, &lastscan);
	if(fres == FR_OK) {
		fres = f_remove_lfn(&firstscan, &lastscan, &dclust, TRUE);
		if(fres == FR_OK) {
	        if (!remove_chain(dclust))
	            return FR_RW_ERROR;	// Remove the cluster chain
		}
	}

    // purge
	if (!move_window(0))
	    return FR_RW_ERROR;

	return fres;
}

#if 0 // WORK IN PROGRESS
FRESULT f_rename_entry (
		DIR *scan,         		/* opened directory to rename entry in */
		char *name,				/* entry name to replace */
		ENTRYADMIN *entryadmin  /* All you need to know about the entry to replace it with */
)
{
	BYTE *dir_old, *dir_new, *p;
	BOOL exists;
    FRESULT res;

	res = find_lfn(scan, entryadmin->name_str, &firstscan, &lastscan);
	if(res == FR_OK) {
		if(!entryadmin->replace) {
			return FR_EXISTS;
		}
	}

	/* see if what is to replace is there */
	res = find_lfn(scan, name, &dir_old, &tempscan, &tempscan);
	if(res == FR_NO_FILE) {
		return res;
	}

	if(exists) {
		/*unlink it */
		memcpy(&dummy_scan, scan, sizeof(dummy_scan));
		res = f_unlink_entry(&dummy_scan, entryadmin->name_str);
	    if(res != FR_OK)
       		return res;
	}
	
	/* pray this works as you have just unlinked an entry */
    res = create_lfn(scan, entryadmin->name_str, &dir_new);
	if(res == FR_OK) 
	{
		*(dir_new+11) = *(dir_old+11);
	    *(WORD*)(dir_new+20) = *(WORD*)(dir_old+20);
		*(WORD*)(dir_new+26) = *(WORD*)(dir_old+26);
		*(DWORD*)(dir_new+28) = *(DWORD*)(dir_old+28);

		/* unlink the old entry */
	}

	return res;
}
#endif /* 0 */

FRESULT f_rename_direct (
        DIRENTRY *entry,        /* the original file to rename */
		DIR *scan,         		/* opened directory to rename entry in */
		char *name,				/* new filename */
)
{
	BYTE *dir_old, *dir_new;
    FRESULT res;
    
    BYTE flags;
    WORD clust_hi;
    WORD clust_lo;
    DWORD filelen;
    DWORD dummy;
    
    // does the new file name already exist? If so, error
	res = find_lfn(scan, name, &firstscan, &lastscan);
	if(res == FR_OK) {     // cannot create two files with the same name in one dir
		return FR_EXISTS;
	}

    // now try to find the cluster in the directory to copy params from into new entry    
	res = find_scans_fcluster(scan, entry->fcluster, &firstscan, &lastscan);
	if(res != FR_OK) {
        return res;
	}

    // last scan now points to the entry in the dir that holds our actual 8.3 entry
    dir_old = (BYTE *)&(FatFs_win[(lastscan.index & 15) * 32]);

    // save the necessary info
    flags = dir_old[11];
    clust_hi = *(WORD*)(dir_old+20);
    clust_lo = *(WORD*)(dir_old+26);
    filelen  = *(DWORD*)(dir_old+28);

    // remove old name
	res = f_remove_lfn(&firstscan, &lastscan, &dummy, FALSE);
	if(res == FR_OK) {
        // apparently, invalidating the old name was ok, so now create a new entry
        // that points to the old file.	
        res = create_lfn(scan, name, &dir_new);
    	if(res == FR_OK) 
    	{
    		*(dir_new+11) = flags;
    	    *(WORD*)(dir_new+20) = clust_hi;
    		*(WORD*)(dir_new+26) = clust_lo;
    		*(DWORD*)(dir_new+28) = filelen;
            
    		FatFs_winflag = 1;			
        	if (!move_window(0))
        	    return FR_RW_ERROR;	// Writeback
    	}
    }
	return res;
}

/*
static dump_scan(DIR *s)
{
    printf("SC: %7ld  CL: %7ld  SEC: %7ld  IDX: %d\n", s->sclust, s->clust, s->sect, s->index);
}
*/

/*-----------------------------------------------*/
/* Create file (and open it) with long file name */
FRESULT f_create_file_entry(
    FIL  *fp,           			/* file object to create */
    DIR  *scan,         			/* opened directory to create file in */
    CHAR *name_str,
    DWORD size,
    BOOL replace,
    BOOL append) 			
{
	static DIR	dummy_scan;
	BYTE *dir_new;
    FRESULT res;
    DWORD dclust;
    static DWORD start_cluster;
    static DWORD next_cluster;
    static DWORD dir_sect;
    static WORD  num_clusters;
    
	memcpy(&dummy_scan, scan, sizeof(dummy_scan));
	res = find_lfn(&dummy_scan, name_str, &firstscan, &lastscan);
/*
    printf("After find_lfn '%s'. Fres=%d.\n", name_str, res);
    printf("first scan: \n");
    dump_scan(&firstscan);
    printf("last scan: \n");
    dump_scan(&lastscan);
    printf("Current sector: %ld\n", FatFs_winsect);
*/
	if(res == FR_OK) {
		if((!replace)&&(!append)) {
			return FR_EXISTS;
		}
		else if(replace) {
			printf("replace!\n");
			/* unlink it */
			res = f_remove_lfn(&firstscan, &lastscan, &dclust, TRUE); 
		    if(res != FR_OK)
        		return res;

            // deallocate
	        if (!remove_chain(dclust))
	            return FR_RW_ERROR;	// Remove the cluster chain
		} else if(append) {
            // execute body of f_open_direct
            dir_new = &(FatFs_win[(lastscan.index & 15) * 32]);	/* Pointer to the directory entry */
        	fp->flag       = FA_READ | ((dir_new[11] & AM_RDO) ? 0 : FA_WRITE); 
        	fp->dir_sect   = lastscan.sect; /* Pointer to the directory entry. */
        	fp->dir_ptr    = dir_new;
         	fp->org_clust  = ((DWORD)LD_WORD(dir_new+20) << 16) | LD_WORD(dir_new+26);	/* File start cluster */
        	fp->fsize      = LD_DWORD(dir_new+28);		/* File size */
        	fp->fptr       = 0; /* File ptr */
        	fp->sect_clust = 1;	/* Sector counter */
        
            //dump_hex(dir_new, 32);

        	FatFs_files++;
		    printf("append! (filesize = %ld)\n", fp->fsize);

            return f_lseek(fp, fp->fsize); // go to end of file           
  	    }
	}

    res = create_lfn(scan, name_str, &dir_new);

    if(res != FR_OK)
        return res;

    start_cluster = 0L;

    if(size) { // reserve space in advance
        dir_sect = FatFs_winsect;

        num_clusters = ((size + 511) >> 9);  // number of sectors
        num_clusters += FatFs_sects_clust - 1; // for rounding
        num_clusters /= FatFs_sects_clust;
        printf("Number of clusters to create: %d.\n", num_clusters);
        start_cluster = create_chain(0L);
        if(!start_cluster)
            return FR_DISK_FULL;
        num_clusters--;
        next_cluster = start_cluster;
        while(num_clusters) {
            next_cluster = create_chain(next_cluster);
            num_clusters--;
            if(!start_cluster)
                return FR_DISK_FULL;
        }
        move_window(dir_sect);
    }

	ST_WORD(dir_new+26, start_cluster);		/* Table start cluster */
	ST_WORD(dir_new+20, start_cluster >> 16);

	FatFs_winflag = 1;
	if (!move_window(0)) return FR_RW_ERROR;

	fp->flag       = FA_WRITE|FA_READ;
	fp->dir_sect   = FatFs_winsect;		/* Pointer to the directory entry */
	fp->dir_ptr    = dir_new;
	fp->org_clust  = start_cluster;     /* File start cluster */
	fp->fsize      = size;              /* File size */
	fp->fptr       = 0;		    		/* File ptr */
	fp->sect_clust = 1;					/* Sector counter */

	FatFs_files++;

    return FR_OK;
}

FRESULT f_create_dir_entry(
    DIR  *scan,         		/* opened directory to create file in */
	CHAR *name_str,
	BOOL replace)
{
	static DIR	dummy_scan;
    BYTE *p;
    DWORD dclust, tim;

	FRESULT res;
	memcpy(&dummy_scan, scan, sizeof(dummy_scan));
	
	res = find_lfn(&dummy_scan, name_str, &firstscan, &lastscan);
	if(res == FR_OK) {
		if(!replace) {
			return FR_EXISTS;
		}
		else {
			printf("replace!\n");
			/*unlink it */
			res = f_remove_lfn(&firstscan, &lastscan, &dclust, TRUE);
		    if(res != FR_OK)
        		return res;

            // deallocate
	        if (!remove_chain(dclust))
	            return FR_RW_ERROR;	// Remove the cluster chain
		}

	}
    
    res = create_lfn(scan, name_str, &p);
    if(res != FR_OK)
        return res;

    res = init_dir(scan->sclust, &dclust);
    
	FatFs_winflag = 1;
	if (!move_window(0)) return FR_RW_ERROR;

	tim = get_fattime();
	*(p+11) = AM_DIR;
	*(p+12) = 0;
	memset(p+13, 0, 32-13);
	ST_DWORD(p+22, tim);		/* Crated time */
	ST_WORD(p+26, dclust);		/* Table start cluster */
	ST_WORD(p+20, dclust >> 16);
	FatFs_winflag = 1;

	if (!move_window(0)) return FR_RW_ERROR;
    return FR_OK;
}

#endif /* _FS_READONLY */
