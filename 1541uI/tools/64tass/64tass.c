/*
    Turbo Assembler 6502/65C02/65816
    Copyright (C) <year>  <name of author>

   6502/65C02 Turbo Assembler  Version 1.3
   (c)1996 Taboo Productions
  
   6502/65C02 Turbo Assembler  Version 1.35  ANSI C port
   (c)2000 [BiGFooT/BReeZe^2000]
  
   6502/65C02/65816 Turbo Assembler  Version 1.4x
   (c)2001-2002 Soci/Singular (soci@c64.rulez.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#define _GNU_SOURCE
#define _MAIN_C_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <time.h>

#include <unistd.h>
#include "opcodes.h"
#include "misc.h"

static char *mnemonic,*opcode;    //mnemonics and opcodes

#define linelength 4096
#define nestinglevel 256
int errors=0,warnings=0;
long sline;      //current line
long cline;      //caller line
char sname[80];  //current source's name
char cname[80];  //caller source's name
static unsigned long low_mem,top_mem;
static unsigned long all_mem;
static int pass=0;      //pass
static int listing=0;   //listing
static int foundopcode;   //listing
static unsigned char* mem64=NULL;//c64mem
static unsigned char* mmap=NULL; //c64 memory map
unsigned long address=0,l_address=0; //address, logical address
char pline[linelength];  //current line data
static char llist[linelength];  //current line for listing
static int lpoint;              //position in current line
static char ident[linelength];  //identifier (label, etc)
static char varname[linelength];//variable (same as identifier?)
static char path[80];           //path
static int pagelo=-1;           //still in same page?
static FILE* flist;             //listfile
static char lastl=0;            //for listing (skip one line
static int logisave=0;          // number of nested .logical
static unsigned long* logitab;  //.logical .here
static char longaccu=0,longindex=0,scpumode=0;
static unsigned char databank=0;
static unsigned int dpage=0;
static char fixeddig;
static char procname[linelength];

static char s_stack[256];
static struct {long val; char sgn;} e_stack[256];
static long v_stack[256];
static int ssp,esp,vsp;

static char waitfor[nestinglevel];
static char skipit[nestinglevel];
static int waitforp=0;

static int last_mnem;

int labelexists;
                       // 0       1       2       3       4        5       6       7    8            9
static char* command[]={"byte","text","word"   ,"offs"  ,"macro"  ,"endm","for" ,"next","if"     ,"else",
                          "fi","rept","include","binary","comment","endc","page","endp","logical","here",
			  "as","al"  ,"xs"     ,"xl"    ,"error"  ,"proc","pend", "databank","dpage","fill","global"};
#define COMMANDS 31

// ---------------------------------------------------------------------------

void status() {
    fprintf(stdout,"Error messages:    ");
    if (errors) fprintf(stdout,"%i\n",errors); else fprintf(stdout,"None\n");
    fprintf(stdout,"Warning messages:  ");
    if (warnings) fprintf(stdout,"%i\n",warnings); else fprintf(stdout,"None\n");
    fprintf(stdout,"Passes:            %i\n",pass);
    fprintf(stdout,"Range:             ");
    if (low_mem<=top_mem) fprintf(stdout,"$%06lx-$%06lx\n\n",low_mem,top_mem);else fprintf(stdout,"None\n");

    if (mem64) free(mem64);			// free codemem
    if (mmap) free(mmap);				// free memorymap

    tfree();
}

// ---------------------------------------------------------------------------

void readln(FILE* fle) {
    int i=0,q=0;
    char ch;
  
    pline[0]=foundopcode=0;
    fgets(pline,sizeof(pline),fle);
    if (!pline[0]) strcpy(pline,"\n");
    sline++;
    if (listing) strcpy(llist,pline);
    if (((ch=pline[strlen(pline)-1])!=13) && (ch!=10) && (!feof(fle))) err_msg(ERROR_LINE_TOO_LONG,NULL);
    while (((ch=pline[i])!=0) && (ch!=13) && (ch!=10)) {
	if (ch=='"') q=!q;
	else if (!q)
	{
	    if (ch==9) pline[i]=' '; //tab
	    else if (ch==';') break;
	}
	i++;
    }
    while (pline[i-1]==' ') i--;
    pline[i]=lpoint=0;
}

// ---------------------------------------------------------------------------

void pokeb(unsigned char byte) {

    if (fixeddig)
    {
	if (arguments.nonlinear) mmap[address>>3]|=(1<<(address & 7));
	if (address<low_mem) low_mem=address;
	if (address>top_mem) top_mem=address;
	if (address<=all_mem) mem64[address]=byte;
    }
    address++;l_address++;
    if (address>all_mem) {
	if (fixeddig) err_msg(ERROR_TOP_OF_MEMORY,NULL);
	address=0;
    }
    if ((fixeddig) && (scpumode)) if (!(address & 0xffff)) err_msg(ERROR___BANK_BORDER,NULL);
}

// ---------------------------------------------------------------------------
int what(int *tempno) {
    char ch;
    int no;

    ignore();
    switch (ch=whatis[(int)here()]) {
    case WHAT_COMMAND:
	{
            lpoint++;
	    for (no=0; no<COMMANDS; no++)
	    {
		for (ch=0; ch<strlen(command[(int)no]); ch++)
		    if (command[(int)no][(int)ch]!=lowcase(pline[lpoint+ch])) break;
		if (ch==strlen(command[(int)no]))
		{
		    if (((ch=lowcase(pline[lpoint+strlen(command[(int)no])]))<'a' ||
			 ch>'z') && ch!='_') {
			lpoint+=strlen(command[(int)no]);
			*tempno=no;
			return WHAT_COMMAND;
		    }
		}
	    }
	    *tempno=no;
	    return 0;
	}
    case WHAT_COMA:
	lpoint++;
        ignore();
	switch (lowcase(get()))
	{
	case 'y': return WHAT_Y;
	case 'x': ignore();if (get()==')') return WHAT_XZ; else {lpoint--; return WHAT_X;}
	case 's': ignore();if (get()==')') return WHAT_SZ; else {lpoint--; return WHAT_S;}
	default: lpoint--;return WHAT_COMA;
	}
    case WHAT_CHAR:
	if (!foundopcode) {
	    char s1,s2,s3,*p;
	    int also=0,felso,s4,elozo;

	    ch=lowcase(here());
	    no=(felso=last_mnem)/2;
	    if (((s1=lowcase(pline[lpoint+3]))==0x20 || !s1) &&
		(s2=lowcase(pline[lpoint+1])) && (s3=lowcase(pline[lpoint+2])))
		for (;;) {  // do binary search
		    if (!(s4=ch-*(p=mnemonic+no*3)))
			if (!(s4=s2-*(++p)))
			    if (!(s4=s3-*(++p)))
			    {
				lpoint+=3;
				*tempno=no;foundopcode=1;
				return WHAT_MNEMONIC;
			    }

		    elozo=no;
		    if (elozo==(no=((s4>0) ? (felso+(also=no)) : (also+(felso=no)) )/2)) break;
		}
	}
    case WHAT_LBL:
            *tempno=1;return WHAT_EXPRESSION;
    case WHAT_EXPRESSION://tempno=1 if label, 0 if expression
	    *tempno=0;return WHAT_EXPRESSION;
    default:lpoint++;return ch;
    }
 }

int get_ident() {
    int v,code;
    int i=0;
    char ch;
 
    if ((v=what(&code))!=WHAT_EXPRESSION || !code) {
	err_msg(ERROR_EXPRES_SYNTAX,NULL);
	return 1;
    }
    if (arguments.casesensitive) {
	while ((whatis[ch=(char)here()]==WHAT_CHAR) || (ch>='0' && ch<='9') || ch=='.' || ch=='_') {
	    ident[i++]=ch;
	    lpoint++;
	}
    } else {
	while (((ch=lowcase(pline[lpoint]))>='a' && ch<='z') || (ch>='0' && ch<='9') || ch=='.' || ch=='_') {
	    ident[i++]=ch;
	    lpoint++;
	}
    }
    ident[i]=0;
    return 0;
}

long get_num(int *cd) {
    long val=0;            //md=0, define it, md=1 error if not exist
    struct slabel* tmp;

    unsigned char ch;
    *cd=0;   // 0=unknown stuff, 1=ok, 2=exist, unuseable

    ignore();
    switch (ch=get()) {
    case '$':
	{
	    ignore();
	    while (((ch=lowcase(get()))>='0' && ch<='9') ||
		   (ch>='a' && ch<='f')) {
		if (val>0x7ffffffl) {err_msg(ERROR_CONSTNT_LARGE,NULL); return 0;}
		val=(val<<4)+(ch=ch<='9' ? ch-'0' : ch-'a'+10);
	    }
	    lpoint--;
	    *cd=1;
	    return val;
	}
    case '%':
	{
	    ignore();
	    while (((ch=get()) & 0xfe)=='0') {
		if (val>0x3fffffffl) {err_msg(ERROR_CONSTNT_LARGE,NULL); return 0;}
		val=(val<<1)+ch-'0';
	    }
	    lpoint--;
	    *cd=1;
	    return val;
	}
    case '"':
	{
	    if ((ch=petascii(get()))) {
		if (get()=='"') {*cd=1; return ch;}
	    }
	    err_msg(ERROR_EXPRES_SYNTAX,NULL);
	    return 0;
	}
    case '*': *cd=1; return l_address;
    default:
	{
	    static char ident2[linelength];
	    lpoint--;
	    if ((ch>='0') && (ch<='9')) { //decimal number...
		while (((ch=get())>='0') && (ch<='9')) {
		    if (val>(0x7fffffffl-ch+'0')/10) {err_msg(ERROR_CONSTNT_LARGE,NULL); return 0;}
		    val=(val*10)+ch-'0';
		}
		lpoint--;
		*cd=1;
		return val;
	    }
	    if (get_ident()) return 0; //label?
	    if (procname[0]) {
		strcpy(ident2,procname);
		strcat(ident2,".");
		strcat(ident2,ident);
		tmp=find_label(ident2);     //ok, local label
	    } else tmp=NULL;
            if (!tmp) tmp=find_label(ident);  //ok, global label
	    if (pass==1) {
		if (tmp) if (tmp->ertelmes) {*cd=1;tmp->proclabel=0;tmp->used=1;return tmp->value;}
		*cd=2;return 0;
	    }
	    else {
		if (tmp) {if (tmp->ertelmes) {*cd=1;tmp->proclabel=0;tmp->used=1;return tmp->value;} else {*cd=2;return 0;}}
		err_msg(ERROR___NOT_DEFINED,ident); //never reached
		return 0;
	    }
	}
    }
}

int priority(char ch) {
    switch (ch)
    {
    case '(':return 0;
    case 'l':          // <
    case 'h':return 5; // >
    case '=':
    case '<':
    case '>':
    case 'o':          // !=
    case 'g':          // >=
    case 's':return 10;// <=
    case '+':
    case '-':return 15;
    case '*':
    case '/':
    case 'u':return 20;// mod
    case '|':
    case '^':return 25;
    case '&':return 30;
    case 'm':          // <<
    case 'd':return 35;// >>
    case 'n':          // -
    case 'i':          // ~
    case 't':return 40;// !
    default:return 0;
    }
}

void pushs(char ch) {
    if ((ch=='n' || ch=='t' || ch=='i' || ch=='l' || ch=='h') && ssp &&
	priority(s_stack[ssp-1])==priority(ch)) { s_stack[ssp++]=ch; return; }
    if (!ssp || priority(s_stack[ssp-1])<priority(ch)) {
	s_stack[ssp++]=ch;
	return;
    }
    while (ssp && priority(s_stack[ssp-1])>=priority(ch)) e_stack[esp++].sgn=s_stack[--ssp];
    s_stack[ssp++]=ch;
}

char val_length(long val)
{
        if (val<0) return 3;
    	if (val<0x100) return 0;
        if (val<0x10000) return 1;
	if (val<0x1000000) return 2;
        return 3;
}

long get_exp(int *wd, int *df,int *cd) {// length in bytes, defined
    long val;
    int i,cod,nd=0,tp=0;
    char ch;

    ssp=esp=0;
    *wd=3;    // 0=byte 1=word 2=long 3=negative/too big
    *df=1;    // 1=result is ok, result is not yet known
    *cd=0;    // 0=error

    ignore();
    if (pline[lpoint]=='@')
    {
	switch (lowcase(pline[++lpoint]))
	{
	case 'b':*wd=0;break;
	case 'w':*wd=1;break;
	case 'l':*wd=2;break;
	default:err_msg(ERROR______EXPECTED,"@B or @W or @L"); return 0;
	}
        lpoint++;
	ignore();
    }
    if (pline[lpoint]=='(') tp=1;
    for (;;) {
	if (!nd) {
	    ignore();
	    if ((ch=get())=='(') {s_stack[ssp++]='('; continue;}
	    if (ch=='+') continue;
	    if (ch=='-') {pushs('n'); continue;}
	    if (ch=='!') {pushs('t'); continue;}
	    if (ch=='~') {pushs('i'); continue;}
	    if (ch=='<') {pushs('l'); continue;}
	    if (ch=='>') {pushs('h'); continue;}
	    lpoint--;
	    val=get_num(&cod);
	    if (!cod) return 0; //unknown stuff found...
	    if (cod==2) *df=0;  //not yet useable...
	    e_stack[esp].val=val;
	    e_stack[esp++].sgn=' ';
	    nd=1;
	}
	else {
	    ignore();
	    if ((ch=pline[lpoint])=='&' || ch=='|' || ch=='^' ||
		ch=='*' || ch=='/' || ch=='+' || ch=='-' || ch=='=' ||
		ch=='<' || ch=='>' || (ch=='!' && pline[lpoint+1]=='=')) {
		if (tp) tp=1;
		if ((ch=='<') && (pline[lpoint+1]=='<')) {pushs('m'); lpoint++;}
		else if ((ch=='>') && (pline[lpoint+1]=='>')) {pushs('d'); lpoint++;}
		else if ((ch=='>') && (pline[lpoint+1]=='=')) {pushs('g'); lpoint++;}
		else if ((ch=='<') && (pline[lpoint+1]=='=')) {pushs('s'); lpoint++;}
		else if ((ch=='/') && (pline[lpoint+1]=='/')) {pushs('u'); lpoint++;}
		else if (ch=='!') {pushs('o'); lpoint++;}
		else pushs(ch);
		nd=0;
		lpoint++;
		continue;
	    }
	    if (ch==')') {
		while ((ssp) && (s_stack[ssp-1]!='('))
		    e_stack[esp++].sgn=s_stack[--ssp];
		lpoint++;
		if (ssp==1 && tp) tp=2;
		if (!ssp) {err_msg(ERROR_EXPRES_SYNTAX,NULL); return 0;}
		ssp--;
		continue;
	    }
	    while ((ssp) && (s_stack[ssp-1]!='('))
		e_stack[esp++].sgn=s_stack[--ssp];
	    if (!ssp) {
		if (tp==2) *cd=3; else *cd=1;
		break;
	    }
	    if (ssp>1) {err_msg(ERROR_EXPRES_SYNTAX,NULL); return 0;}
	    if (tp) *cd=2;
	    else {err_msg(ERROR_EXPRES_SYNTAX,NULL); return 0;}
	    break;
	}
    }
    vsp=0;
    for (i=0; i<esp; i++) {
	if ((ch=e_stack[i].sgn)==' ')
	    v_stack[vsp++]=e_stack[i].val;
	else {
	    if (ch=='l') {v_stack[vsp-1]&=255; continue;}
	    if (ch=='h') {v_stack[vsp-1]/=256; v_stack[vsp-1]&=255; continue;}
	    if (ch=='n') {v_stack[vsp-1]=-v_stack[vsp-1]; continue;}
	    if (ch=='i') {v_stack[vsp-1]=~v_stack[vsp-1]; continue;}
	    if (ch=='t') {v_stack[vsp-1]=!v_stack[vsp-1]; continue;}
	    if (ch=='=') {v_stack[vsp-2]=v_stack[vsp-2]==v_stack[vsp-1]; vsp--; continue;}
	    if (ch=='o') {v_stack[vsp-2]=v_stack[vsp-2]!=v_stack[vsp-1]; vsp--; continue;}
	    if (ch=='<') {v_stack[vsp-2]=v_stack[vsp-2]<v_stack[vsp-1]; vsp--; continue;}
	    if (ch=='>') {v_stack[vsp-2]=v_stack[vsp-2]>v_stack[vsp-1]; vsp--; continue;}
	    if (ch=='g') {v_stack[vsp-2]=v_stack[vsp-2]>=v_stack[vsp-1]; vsp--; continue;}
	    if (ch=='s') {v_stack[vsp-2]=v_stack[vsp-2]<=v_stack[vsp-1]; vsp--; continue;}
	    if (ch=='*') {v_stack[vsp-2]*=v_stack[vsp-1]; vsp--; continue;}
	    if (ch=='/') {v_stack[vsp-2]/=v_stack[vsp-1]; vsp--; continue;}
	    if (ch=='u') {v_stack[vsp-2]%=v_stack[vsp-1]; vsp--; continue;}
	    if (ch=='+') {v_stack[vsp-2]+=v_stack[vsp-1]; vsp--; continue;}
	    if (ch=='-') {v_stack[vsp-2]-=v_stack[vsp-1]; vsp--; continue;}
	    if (ch=='&') {v_stack[vsp-2]&=v_stack[vsp-1]; vsp--; continue;}
	    if (ch=='|') {v_stack[vsp-2]|=v_stack[vsp-1]; vsp--; continue;}
	    if (ch=='^') {v_stack[vsp-2]^=v_stack[vsp-1]; vsp--; continue;}
	    if (ch=='m') {v_stack[vsp-2]<<=v_stack[vsp-1]; vsp--; continue;}
	    if (ch=='d') {v_stack[vsp-2]>>=v_stack[vsp-1]; vsp--; continue;}
	}
    }
    val=v_stack[0];
    if (*df) {
	switch (*wd)
	{
	case 0:if (val>0xff) val=-1;break;
	case 1:if (val>0xffff) val=-1;break;
	case 2:if (val>0xffffff) val=-1;break;
	default:return val;
	}
        if (val>=0) return val;
	err_msg(ERROR_CONSTNT_LARGE,NULL);
	*cd=0;
    }
    return 0;
}

void wait_cmd(FILE* fil,int no)
{
    int wrap=waitforp;
    int pr,wh;
    long lin,pos;

    for (;;) {
	if (feof(fil)) {
	    char nc[20];
	    strcpy(nc,".");
	    strcat(nc,command[no]);
	    err_msg(ERROR______EXPECTED,nc);
	    return;
	}
	if (no==26) { //.pend
	    lin=sline;
	    pos=ftell(fil);
	}
	readln(fil);
	if ((wh=what(&pr))==WHAT_EXPRESSION) {
	    get_ident();   //skip label
	    wh=what(&pr);
	}
	if (wh==WHAT_COMMAND) {
	    if (pr==no && wrap==waitforp) return;
	    switch (pr)
	    {
	    case 6:waitfor[++waitforp]='n';break;//.for
	    case 7:if (waitfor[waitforp]=='n') waitforp--;break;//.next
	    case 8:waitfor[++waitforp]='e';break;//.if
	    case 9:if (waitfor[waitforp]=='e') waitfor[waitforp]='f';break;//.else
	    case 10:if (waitfor[waitforp]=='e' || waitfor[waitforp]=='f') waitforp--;break;//.fi
	    case 11:waitfor[++waitforp]='n';break;//.rept
	    case 25:if (no==26 && wrap==waitforp) {sline=lin;fseek(fil,pos,SEEK_SET);return;}break;// .proc
	    }
	}
    }
}

int get_path() {
    int i=0,q=1;
    ignore();
    if (pline[lpoint]=='\"') {lpoint++;q=0;}
    while (pline[lpoint] && (pline[lpoint]!='\"' || q) && i<80) path[i++]=get();
    if (i==80 || (!q && pline[lpoint]!='\"')) {err_msg(ERROR_GENERL_SYNTAX,NULL); return 1;}
    if (!q) lpoint++;
    path[i]=0;
    ignore();if (get()) {err_msg(ERROR_EXTRA_CHAR_OL,NULL); return 1;}
    return 0;
}

void mtranslate(char* mpr, int nprm) //macro parameter expansion
{
    int q,p,pp,i,j;
    char ch;
    char tmp[linelength];

    strcpy(tmp,pline);
    q=p=0;
    for (i=0; (ch=tmp[i]); i++) {
	if (ch=='"') q=!q;
	else if (!q) {
	    if (ch=='\\') {
		if (((ch=lowcase(tmp[i+1]))>='1' && ch<='9') || (ch>='a' && ch<='z')) {
		    if ((ch=(ch<='9' ? ch-'1' : ch-'a'+9))>=nprm) {err_msg(ERROR_MISSING_ARGUM,NULL); break;}
		    for (pp=j=0; j<ch; j++) while (mpr[pp++]); //skip parameters
		    while (mpr[pp] && p<linelength) pline[p++]=mpr[pp++];//copy
		    if (p>=linelength) err_msg(ERROR_LINE_TOO_LONG,NULL);
		    i++;continue;
		} else ch='\\';
	    }
	}
	pline[p++]=ch;
	if (p>=linelength) err_msg(ERROR_LINE_TOO_LONG,NULL);
    }
    pline[p]=0;
}

void compile(char* nam,long fpos,long lne,char tpe,char* mprm,int nprm,FILE* fin) // "",0
{
    char mparams[256];
    FILE* fil;
  
    int wht,w,d,c,i;
    int prm=0;
    long val;

    char ch;

    long pos,lin,cnt,oldpos=-1;

    char snum[12];
    int fflen;

    struct slabel* tmp;
    struct smacro* tmp2;

    struct stat filestat;

    strcpy(cname,sname);
    strcpy(sname,nam);
    cline=sline;
    sline=lne;
    if (fin) oldpos=ftell(fin);
    else {
        char volt;
	if ((fin=openfile(nam,&volt))==NULL) err_msg(ERROR_CANT_FINDFILE,nam);
	if (volt) {
	    if (tpe==1) oldpos=ftell(fin);
	    else err_msg(ERROR_FILERECURSION,NULL);
	}
     }
    if (fseek(fin,fpos,SEEK_SET)) err_msg(ERROR_CANT_FINDFILE,"[Compile - fseek()]");
    for (;;) {
	if (feof(fin))
	{
	    switch (tpe) {
	    case 1:err_msg(ERROR______EXPECTED,".ENDM"); break;
	    case 2:err_msg(ERROR______EXPECTED,".NEXT");
	    }
	    break;
	}

	readln(fin);
	if (nprm) mtranslate(mprm,nprm); //expand macro parameters, if any

	if ((wht=what(&prm))==WHAT_EXPRESSION && skipit[waitforp]) { //skip things if needed
	    if (!prm) {err_msg(ERROR_GENERL_SYNTAX,NULL); continue;} //not label
	    get_ident();                                           //get label
	    if ((wht=what(&prm))==WHAT_EQUAL) { //variable
		strcpy(varname,procname);
		if (procname[0]) strcat(varname,".");
		strcat(varname,ident);
		val=get_exp(&w,&d,&c); //ellenorizve.
		if (!c) continue;
		if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); continue;}
		ignore();if (here()) {err_msg(ERROR_EXTRA_CHAR_OL,NULL); continue;}
		tmp=new_label(varname);
		if (pass==1) {
		    if (labelexists) {
			err_msg(ERROR_DOUBLE_DEFINE,varname);
			continue;
		    }
		    else {
			tmp->proclabel=0;tmp->used=0;
			if (d) {tmp->value=val;tmp->ertelmes=1;}
                        else tmp->ertelmes=0;
		    }
		}
		else {
		    if (labelexists) {
			if (tmp->ertelmes) {
			    if (tmp->value!=val) {
                                tmp->value=val;
				fixeddig=0;
			    }
			}
			else {
			    if (d) {tmp->value=val;tmp->ertelmes=1;}
			}
		    }
		}
                continue;
	    }
	    if (wht==WHAT_COMMAND && prm==4) { // .macro
		ignore();if (here()) {err_msg(ERROR_EXTRA_CHAR_OL,NULL); continue;}
		tmp2=new_macro(ident);
		if (labelexists) {
		    if (pass==1) {err_msg(ERROR_DOUBLE_DEFINE,ident); continue;}
		}
		else {
		    tmp2->point=ftell(fin);
		    tmp2->lin=sline;
		}
		wait_cmd(fin,5); //.endm
		continue;
	    }
	    if (listing && arguments.source && ((wht==WHAT_COMMAND && prm>2) || wht==WHAT_STAR || wht==WHAT_EOL)) {
		if (lastl==2) {fputc('\n',flist);lastl=1;}
		fprintf(flist,".%06lx               ",address);
		if (arguments.monitor) fprintf(flist,"             ");
		fprintf(flist,"%s\n",ident);
	    }
	    if (prm==25 || prm==30) tmp=new_label(ident); //.proc or .global
	    else {
		strcpy(varname,procname);
		if (procname[0]) strcat(varname,".");
		strcat(varname,ident);
		tmp=new_label(varname);
	    }
	    if (pass==1) {
		if (labelexists) {
		    err_msg(ERROR_DOUBLE_DEFINE,ident);
		    continue;
		}
		else {
                    tmp->ertelmes=1;tmp->used=0;
		    tmp->value=l_address;
		    if (wht==WHAT_COMMAND && prm==25) tmp->proclabel=1; else tmp->proclabel=0;
		}
	    }
	    else {
		if (labelexists) {
		    if (tmp->ertelmes) {
			if (tmp->value!=l_address) {
			    tmp->value=l_address;
			    fixeddig=0;
			}
		    }
		}
	    }
	}
	switch (wht) {
	case WHAT_STAR:if (skipit[waitforp]) //skip things if needed
	    {
		ignore();if (get()!='=') {err_msg(ERROR______EXPECTED,"="); break;}
		val=get_exp(&w,&d,&c);if (!d) fixeddig=0;
		if (!c) break;
		if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}
		ignore();if (here()) goto extrachar;
		if (val>all_mem || val<0) {err_msg(ERROR_CONSTNT_LARGE,NULL); break;}
		address=l_address=(unsigned)val;
		lastl=0;
		break;
	    }
	case WHAT_EOL: break;
	case WHAT_COMMAND:
	    {
		char lcol,kiirva; //for listing
                ignore();
		if (prm==10) // .fi
		{
                    if (waitfor[waitforp]!='e' && waitfor[waitforp]!='f') {err_msg(ERROR______EXPECTED,".IF"); break;}
		    if (here()) goto extrachar;
		    waitforp--;
                    break;
		}
		if (prm==9) { // .else
		    if (waitfor[waitforp]=='f') {err_msg(ERROR______EXPECTED,".FI"); break;}
		    if (waitfor[waitforp]!='e') {err_msg(ERROR______EXPECTED,".IF"); break;}
		    if (here()) goto extrachar;
		    skipit[waitforp]^=1;
		    waitfor[waitforp]='f';
                    break;
		}
		if (prm==8) { // .if
		    if (!skipit[waitforp]) {wait_cmd(fin,10);break;}
		    val=get_exp(&w,&d,&c); //ellenorizve.
		    if (!c) break;
		    if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}
		    ignore();if (here()) goto extrachar;
                    if (!d) {err_msg(ERROR___NOT_DEFINED,"argument used in .IF");wait_cmd(fin,10);break;}
		    waitfor[++waitforp]='e';
		    if (val) skipit[waitforp]=1; else
                        skipit[waitforp]=0;
		    break;
		}
                if (!skipit[waitforp]) break; //skip things if needed
		if (prm<2) {    // .byte .text
		    if (listing) {
			if (lastl!=2) {fputc('\n',flist);lastl=2;}
			fprintf(flist,">%06lx  ",address);
			lcol=25;
                        kiirva=1;
		    }
		    for (;;) {
			ignore();
			if ((ch=here())=='"') {
			    if (pline[lpoint+1] && pline[lpoint+1]!='"' && pline[lpoint+2]=='"') goto textconst;
                            lpoint++;
			    for (;;) {
				if (!(ch=get())) {err_msg(ERROR______EXPECTED,"\""); break;}
				if (ch=='"') {
				    if (pline[lpoint]=='"') lpoint++; else break;
				}
				if (listing) {
				    if (lcol==1) {
					if (kiirva) {fprintf(flist,"  %s",llist);kiirva=0;} else fputc('\n',flist);
					fprintf(flist,">%06lx  ",address);lcol=25;
				    }
				    fprintf(flist,"%02x ",(unsigned char)petascii(ch));
				    lcol-=3;
				}
				pokeb(petascii(ch));
			    }
			    ignore();if ((ch=get())==',') continue;
			    if (ch) err_msg(ERROR______EXPECTED,",");
			    break;
			}
			if (ch=='^') {
                            lpoint++;
			    val=get_exp(&w,&d,&c);if (!d) fixeddig=0;
			    if (!c) break;
			    if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}

			    sprintf(snum,"%ld",val);

			    for(i=0; snum[i]; i++) {
				if (listing) {
				    if (lcol==1) {
					if (kiirva) {fprintf(flist,"  %s",llist);kiirva=0;} else fputc('\n',flist);
					fprintf(flist,">%06lx  ",address);lcol=25;
				    }
				    fprintf(flist,"%02x ",(unsigned char)snum[i]);
				    lcol-=3;
				}
				pokeb(snum[i]);
			    }
			    ignore();if ((ch=get())==',') continue;
			    if (ch) err_msg(ERROR______EXPECTED,",");
			    break;
			}
		    textconst:
			val=get_exp(&w,&d,&c); //ellenorizve.
			if (!c) break;
			if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}
			if (d && (val>0xff || val<0)) {err_msg(ERROR_CONSTNT_LARGE,NULL); break;}
			if (listing) {
			    if (lcol==1) {
				if (kiirva) {fprintf(flist,"  %s",llist);kiirva=0;} else fputc('\n',flist);
				fprintf(flist,">%06lx  ",address);lcol=25;
			    }
			    fprintf(flist,"%02x ",(unsigned char)val);
			    lcol-=3;
			}
			pokeb(val);
			ignore();if ((ch=get())==',') continue;
			if (ch) err_msg(ERROR______EXPECTED,",");
			break;
		    }
		}
		if (prm==2) { // .word
		    if (listing) {
			if (lastl!=2) {fputc('\n',flist);lastl=2;}
			fprintf(flist,">%06lx  ",address);
			lcol=25;
                        kiirva=1;
		    }
		    for (;;) {
			val=get_exp(&w,&d,&c); //ellenorizve.
			if (!c) break;
			if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}
			if (d && (val>0xffff || val<0)) {err_msg(ERROR_CONSTNT_LARGE,NULL); break;}
			if (listing) {
			    if (lcol==1) {
				if (kiirva) {fprintf(flist,"  %s",llist);kiirva=0;} else fputc('\n',flist);
				fprintf(flist,">%06lx  ",address);lcol=25;
			    }
			    fprintf(flist,"%02x %02x ",(unsigned char)val,(unsigned char)(val>>8));
			    lcol-=6;
			}
			pokeb((unsigned char)val);
			pokeb((unsigned char)(val>>8));
			ignore();if ((ch=get())==',') continue;
			if (ch) err_msg(ERROR______EXPECTED,",");
			break;
		    }
		}
		if (prm<3) { // .byte .text .word
		    if (listing)
		    {
			if (arguments.source && kiirva) {
			    for (i=0; i<lcol; i++) fputc(' ',flist);
			    fprintf(flist," %s",llist);
			} else fputc('\n',flist);
		    }
		    break;
		}
		if (prm==3) {   // .offs
		    val=get_exp(&w,&d,&c); //ellenorizve.
		    if (!c) break;
		    if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}
		    ignore();if (here()) goto extrachar;
		    address=(address+(unsigned)val) & all_mem;
		    break;
		}
		if (prm==18) { // .logical
		    val=get_exp(&w,&d,&c);if (!d) fixeddig=0;
		    if (!c) break;
		    if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}
		    ignore();if (here()) goto extrachar;
		    if (val>all_mem || val<0) {err_msg(ERROR_CONSTNT_LARGE,NULL); break;}
		    if (!(logitab=realloc(logitab,(logisave+1)*sizeof(*logitab)))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
		    logitab[logisave++]=l_address-address;
		    l_address=(unsigned)val;
		    break;
		}
		if (prm==19) { // .here
		    if (here()) goto extrachar;
		    if (!logisave) {err_msg(ERROR______EXPECTED,".LOGICAL"); break;}
		    l_address=address+logitab[--logisave];
		    logitab=realloc(logitab,logisave*sizeof(*logitab));
		    break;
		}
		if (prm==20) { // .as
		    if (here()) goto extrachar;
                    if (scpumode) longaccu=0;
		    break;
		}
		if (prm==21) { // .al
		    if (here()) goto extrachar;
                    if (scpumode) longaccu=1;
		    break;
		}
		if (prm==22) { // .xs
		    if (here()) goto extrachar;
		    if (scpumode) longindex=0;
		    break;
		}
		if (prm==23) { // .xl
		    if (here()) goto extrachar;
		    if (scpumode) longindex=1;
		    break;
		}
		if (prm==24) { // .error
		    err_msg(ERROR__USER_DEFINED,&pline[lpoint]);
		    break;
		}
		if (prm==25) { // .proc
		    if (here()) goto extrachar;
		    if (tmp) {
			if (tmp->proclabel && pass!=1 && !procname[0]) wait_cmd(fin,26);//.pend
			else strcpy(procname,ident);
		    }
		    break;
		}
		if (prm==26) { //.pend
		    if (here()) goto extrachar;
                    procname[0]=0;
		    break;
		}
		if (prm==27) { // .databank
		    val=get_exp(&w,&d,&c);if (!d) fixeddig=0;
		    if (!c) break;
		    if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}
		    ignore();if (here()) goto extrachar;
		    if (val_length(val)) {err_msg(ERROR_CONSTNT_LARGE,NULL); break;}
		    if (scpumode) databank=val;
		    break;
		}
		if (prm==28) { // .dpage
		    val=get_exp(&w,&d,&c);if (!d) fixeddig=0;
		    if (!c) break;
		    if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}
		    ignore();if (here()) goto extrachar;
		    if (val_length(val)>1) {err_msg(ERROR_CONSTNT_LARGE,NULL); break;}
		    dpage=val;
		    break;
		}
		if (prm==29) { // .fill
                    long db;
		    val=get_exp(&w,&d,&c);if (!d) fixeddig=0;
		    if (!c) break;
		    if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}
		    if (val>all_mem || val<0) {err_msg(ERROR_CONSTNT_LARGE,NULL); break;}
                    db=val;
		    ignore();if (get()!=',') {err_msg(ERROR______EXPECTED,","); break;}
		    val=get_exp(&w,&d,&c);
		    if (!c) break;
		    if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}
		    if (val_length(val)) {err_msg(ERROR_CONSTNT_LARGE,NULL); break;}
		    ignore();if (here()) goto extrachar;
                    while (db-->0) pokeb((unsigned char)val);
		    break;
		}
		if (prm==30) { //.global
		    if (here()) goto extrachar;
		    break;
		}
		if (prm==5) { // .endm
		    if (tpe==1) { // .macro
			if (here()) goto extrachar;
                        goto end;
		    } else {err_msg(ERROR______EXPECTED,".MACRO"); break;}
		}
		if (prm==7) { // .next
		    if (tpe==2) { //.rept .for
			if (here()) goto extrachar;
                        goto end;
		    } else {err_msg(ERROR______EXPECTED,".FOR or .REPT"); break;}
		}
		if (prm==11) { // .rept
		    val=get_exp(&w,&d,&c);if (!d) fixeddig=0;
		    if (!c) break;
		    if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}
		    ignore();if (here()) goto extrachar;
		    lin=sline;
		    pos=ftell(fin);
		    for (cnt=0; cnt<val; cnt++) {
			compile(nam,pos,lin,2,mprm,nprm,fin);
		    }
	            sline=lin;
	            wait_cmd(fin,7);
		    break;
		}
		if (prm==15) {err_msg(ERROR______EXPECTED,".COMMENT"); break;} // .endc
		if (prm==14) { // .comment
                    if (here()) goto extrachar;
		    wait_cmd(fin,15);
		    break;
		}
		if (prm==12) { // .include
		    if (get_path()) break;
		    lin=sline;
		    compile(path,0,0,0,mprm,nprm,NULL);
                    strcpy(sname,nam);
		    sline=lin;
		    break;
		}
		if (prm==13) { // .binary
		    if (get_path()) break;
		    if (!fixeddig) {
			if (stat(path,&filestat)) {strcpy(sname,path); err_msg(ERROR_CANT_FINDFILE,path);break;}
			if (!filestat.st_ino) {strcpy(sname,path); err_msg(ERROR_CANT_FINDFILE,path);break;}
			fflen=filestat.st_size;
			address+=(unsigned long)fflen;
			l_address+=(unsigned long)fflen;
			break;
		    }
		    if ((fil=fopen(path,"rb"))==NULL) {strcpy(sname,path); err_msg(ERROR_CANT_FINDFILE,path);break;}
		    lcol=0;
		    for (;;) {
			int st=fread(&ch,1,1,fil);
			if (feof(fil)) break;
			if (!st) err_msg(ERROR_CANT_FINDFILE,path);
			if (listing) {
			    if (!lcol) {fprintf(flist,"\n>%06lx  ",address);lcol=16;}
			    fprintf(flist,"%02x ",(unsigned char)ch);
			    lcol--;
			}
			pokeb(ch);
		    }
		    if (listing) fputc('\n',flist);
		    fclose(fil);
		    break;
		}
		if (prm==6) { // .for
		    int apoint=0;
		    char expr[linelength];

		    if ((wht=what(&prm))==WHAT_EXPRESSION && prm==1) { //label
			if (get_ident()) break;
			ignore();if (get()!='=') {err_msg(ERROR_GENERL_SYNTAX,NULL); break;}
			strcpy(varname,ident);
			val=get_exp(&w,&d,&c);
			if (!c) break;
			if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}
			if (!d) {fixeddig=0;break;}
			tmp=new_label(varname);
			if (!labelexists) tmp->proclabel=0;
			tmp->value=val;
			tmp->ertelmes=1;tmp->used=0;
			wht=what(&prm);
		    }
		    if (wht==WHAT_S || wht==WHAT_Y || wht==WHAT_X) lpoint--; else
			if (wht!=WHAT_COMA) {err_msg(ERROR______EXPECTED,","); break;}

		    strcpy(expr,&pline[lpoint]);
		    strcpy(pline,expr);
		    lin=sline;
		    pos=ftell(fin);
		    for (;;) {
			lpoint=0;
			val=get_exp(&w,&d,&c);
			if (!c) break;
			if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}
			if (!d) {fixeddig=0;break;}
			if (!val) break;
			if (!apoint) {
                            ignore();if (get()!=',') {err_msg(ERROR______EXPECTED,","); break;}
			    ignore();if (!get()) continue;
			    lpoint--;
			    if (get_ident()) break;
			    ignore();if (get()!='=') {err_msg(ERROR_GENERL_SYNTAX,NULL); break;}
			    apoint=lpoint;
			    tmp=new_label(ident);
			    if (!labelexists) tmp->proclabel=0;
			    tmp->ertelmes=1;tmp->used=0;
			}
			compile(nam,pos,lin,2,mprm,nprm,fin);
			sline=lin;
			strcpy(pline,expr);
			lpoint=apoint;
			val=get_exp(&w,&d,&c);
			if (!c) break;
			if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}
			ignore();if (here()) goto extrachar;
			tmp->value=val;
		    }
		    wait_cmd(fin,7);
		    break;
		}
		if (prm==17) { // .endp
		    if (here()) goto extrachar;
		    if (pagelo==-1) {err_msg(ERROR______EXPECTED,".PAGE"); break;}
		    if ((l_address>>8)!=pagelo) err_msg(ERROR____PAGE_ERROR,NULL);
		    pagelo=-1;
		    break;
		}
		if (prm==16) { // .page
		    if (here()) goto extrachar;
		    if (pagelo!=-1) {err_msg(ERROR______EXPECTED,".ENDP"); break;}
		    pagelo=(l_address>>8);
		    break;
		}
	    }
	case WHAT_HASHMARK:if (skipit[waitforp]) //skip things if needed
	    {                   //macro stuff
		int ppoint;
		if (get_ident()) {err_msg(ERROR_GENERL_SYNTAX,NULL); break;}
		if (!(tmp2=find_macro(ident))) {err_msg(ERROR___NOT_DEFINED,ident); break;}
		ppoint=nprm=0;
                ignore();
		while ((ch=get())) {
		    if (ch=='^') {
			val=get_exp(&w,&d,&c);if (!d) fixeddig=0;
			if (!c) break;
			if (c==2) {err_msg(ERROR_EXPRES_SYNTAX,NULL); break;}

			sprintf(snum,"%ld",val);

			i=0;while (snum[i]) mparams[ppoint++]=snum[i++];
		    }
		    else if (ch=='"') {
			for (;;) {
			    if (!(ch=get())) {err_msg(ERROR______EXPECTED,"\""); break;}
			    if (ch=='"') {
				if (pline[lpoint]!='"') break;
                                lpoint++;
			    }
			    mparams[ppoint++]=ch;
			}
		    }
		    else {
			do mparams[ppoint++]=ch; while ((ch=get())!=',' && ch);
			lpoint--;
		    }
		    nprm++;
		    mparams[ppoint++]=0;
                    ignore();
                    if (!(ch=get())) break;
		    if (ch!=',') {err_msg(ERROR______EXPECTED,","); break;}
		}
		lin=sline;
		if (strcmp(tmp2->file,nam)) compile(tmp2->file,tmp2->point,tmp2->lin,1,mparams,nprm,NULL);
                    else compile(tmp2->file,tmp2->point,tmp2->lin,1,mparams,nprm,fin);
		strcpy(sname,nam);
                sline=lin;
		break;
	    }
	case WHAT_MNEMONIC:if (skipit[waitforp]) {//skip things if needed
	    int opr,mnem=prm;
	    unsigned char* cnmemonic=&opcode[prm*24]; //current nmemonic
	    char ln;
	    unsigned char cod,longbranch=0;

            ignore();
	    if (!(wht=here())) {
		if ((cod=cnmemonic[ADR_IMPLIED])==____) {err_msg(ERROR_ILLEGAL_OPERA,NULL);break;}
		opr=ADR_IMPLIED;d=ln=0;
	    }  //clc
	    // 1 Db
	    else if (lowcase(wht)=='a' && pline[lpoint+1]==0)
	    {
		if (find_label("a")) err_msg(ERROR_A_USED_AS_LBL,NULL);
		if ((cod=cnmemonic[ADR_ACCU])==____) {err_msg(ERROR_ILLEGAL_OPERA,NULL);break;}
		opr=ADR_ACCU;d=ln=0;// asl a
                lpoint++;
	    }
	    // 2 Db
	    else if (wht=='#') {
                lpoint++;
		val=get_exp(&w,&d,&c); //ellenorizve.
		if (!c) break;
		if (c==2) {err_msg(ERROR_GENERL_SYNTAX,NULL); break;}

		ln=1;
		if ((cod=cnmemonic[ADR_IMMEDIATE])==0xE0 || cod==0xC0 || cod==0xA2 || cod==0xA0) {// cpx cpy ldx ldy
		    if (longindex) ln++;
		}
		else if (cod!=0xC2 && cod!=0xE2) {//not sep rep=all accu
		    if (longaccu) ln++;
		}

		if (d) {
		    if (w==3) w=val_length(val);//auto length
		    if (w>=ln) w=3; //const too large
		    opr=ADR_IMMEDIATE;// lda #
		} else fixeddig=0;
	    }
	    // 3 Db
	    else if (wht=='[') {
                lpoint++;
		val=get_exp(&w,&d,&c); //ellenorizve.
		if (!c) break;
		if (c==2) {err_msg(ERROR_GENERL_SYNTAX,NULL); break;}
		ignore();if (get()!=']') {err_msg(ERROR_GENERL_SYNTAX,NULL); break;}
		if ((wht=what(&prm))==WHAT_Y) {
		    if (d) {
			val-=dpage;
			if (w==3) w=val_length(val);//auto length
			if (w) w=3;// there's no lda [$ffff],y lda [$ffffff],y!
			opr=ADR_ZP_LI_Y;
		    } else fixeddig=0;
                    ln=1; // lda [$ff],y
		}
		else if (wht==WHAT_EOL) {
		    if (cnmemonic[ADR_ADDR_LI]==0xDC) { // jmp [$ffff]
			if (d) {
			    if (w==3) {
				w=val_length(val);//auto length
				if (!w) w=1;
			    }
			    if (w!=1) w=3; // there's no jmp [$ffffff]!
                            opr=ADR_ADDR_LI;
			} else fixeddig=0;
                        ln=2;// jmp [$ffff]
		    }
		    else {
			if (d) {
			    val-=dpage;
			    if (w==3) w=val_length(val);//auto length
			    if (w) w=3; // there's no lda [$ffff] lda [$ffffff]!
                            opr=ADR_ZP_LI;
			} else fixeddig=0;
                        ln=1;// lda [$ff]
		    }             
                    lpoint--;
		}
	    }
	    else {
		if (whatis[wht]!=WHAT_EXPRESSION && whatis[wht]!=WHAT_CHAR && wht!='_' && wht!='*') {err_msg(ERROR_GENERL_SYNTAX,NULL); break;}
		val=get_exp(&w,&d,&c); //ellenorizve.
		if (!c) break;
		if (c==1) {
		    if ((wht=what(&prm))==WHAT_X) {// lda $ff,x lda $ffff,x lda $ffffff,x
			if (w==3) {//auto length
			    if (d) {
				if (cnmemonic[ADR_ZP_X]!=____ && val>=dpage && val<(dpage+0x100)) {val-=dpage;w=0;}
				else if (cnmemonic[ADR_ADDR_X]!=____ && databank==(((unsigned)val) >> 16)) w=1;
				else {
				    w=val_length(val);
				    if (w<2) w=2;
				}
			    } else {fixeddig=0;w=1;}
			} else if (val>=dpage && val<(dpage+0x100)) val-=dpage;
			opr=ADR_ZP_X-w;ln=w+1;
		    }// 6 Db
		    else if (wht==WHAT_Y) {
			if (w==3) {//auto length
			    if (d) {
				if (cnmemonic[ADR_ZP_Y]!=____ && val>=dpage && val<(dpage+0x100)) {val-=dpage;w=0;}
				else if (databank==(((unsigned)val) >> 16)) w=1;
			    } else {fixeddig=0;w=1;}
			} else if (val>=dpage && val<(dpage+0x100)) val-=dpage;
			if (w==2) w=3; // there's no lda $ffffff,y!
			opr=ADR_ZP_Y-w;ln=w+1; // ldx $ff,y lda $ffff,y
		    }// 8 Db
		    else if (wht==WHAT_S) {
			if (d) {
			    if (w==3) w=val_length(val);//auto length
			    if (w) w=3; // there's no lda $ffffff,s or lda $ffff,s!
			    opr=ADR_ZP_S;
			} else fixeddig=0;
			ln=1; // lda $ff,s
		    }// 9 Db
		    else if (wht==WHAT_COMA) { // mvp $10,$20
			int w2,c2;
			long val2;

			if (d) {
			    val2=get_exp(&w2,&d,&c2);
			    if (!c2) break;
			    if (c2==2) {err_msg(ERROR_GENERL_SYNTAX,NULL); break;}
			    if (d) {
				if (w==3) w=val_length(val);//auto length
				if (w2==3) w2=val_length(val2);//auto length
				if (w || w2) w=3; // only byte operands...
				opr=ADR_MOVE;
				val=(val << 8) | val2;
			    } else fixeddig=0;
			} else fixeddig=0;
			ln=2;
		    }// 10 Db
		    else if (wht==WHAT_EOL) {
			if (cnmemonic[ADR_REL]!=____) {
			    if (d) {
				if (fixeddig && (l_address >> 16)!=(((unsigned)val) >> 16)) {err_msg(ERROR_BRANCH_TOOFAR,NULL); break;}
				val=(val-l_address-2) & 0xffff;
				if (val<0xFF80 && val>0x007F) {
				    if (arguments.longbranch && (cnmemonic[ADR_ADDR]==____) &&
					(cnmemonic[ADR_REL]==0x10 || //bpl
					 cnmemonic[ADR_REL]==0x30 || //bmi
					 cnmemonic[ADR_REL]==0x50 || //bvc
					 cnmemonic[ADR_REL]==0x70 || //bvs
					 cnmemonic[ADR_REL]==0x90 || //bcc
					 cnmemonic[ADR_REL]==0xB0 || //bcs
					 cnmemonic[ADR_REL]==0xD0 || //bne
					 cnmemonic[ADR_REL]==0xF0)) {//beq
					longbranch=0x20;val=0x4C03+(((val+l_address+2) & 0xffff) << 16);
					if (fixeddig) err_msg(ERROR___LONG_BRANCH,NULL);
				    } else {
					if (cnmemonic[ADR_ADDR]!=____) {
					    val=(val+l_address+2) & 0xffff;
					    opr=ADR_ADDR;w=1;ln=2;goto brancb;}
					else if (cnmemonic[ADR_REL_L]!=____) {//gra
					    val=(val-1) & 0xffff;
					    opr=ADR_REL_L;w=1;ln=2;goto brancb;}
					else if (fixeddig) {err_msg(ERROR_BRANCH_TOOFAR,NULL); break;}
				    }
				}
				opr=ADR_REL;w=0;// bne
			    } else fixeddig=0;
			    if (longbranch) ln=4; else ln=1;
			brancb:
				ln=ln;
			}
			else if (cnmemonic[ADR_REL_L]!=____) {
			    if (d) {
				if (fixeddig) {
				    if ((l_address >> 16)!=(((unsigned)val) >> 16)) {err_msg(ERROR_BRANCH_TOOFAR,NULL); break;}
				    val=(val-l_address-3) & 0xffff;
				}
				opr=ADR_REL_L;w=1;//brl
			    } else fixeddig=0;
			    ln=2;
			}
			else if (cnmemonic[ADR_LONG]==0x5C || cnmemonic[ADR_LONG]==0x22) {
			    if (w==3) {
				if (cnmemonic[ADR_ADDR]==____) w=2; // jml jsl
				else {
				    if (d) {
					if ((l_address >> 16)==(((unsigned)val) >> 16)) w=1;
					else {
					    w=val_length(val);
					    if (w<2) w=2; // in another bank
					}
				    } else {fixeddig=0;w=1;}
				}
			    }
			    opr=ADR_ZP-w;ln=w+1;
			}
			else {
			    if (w==3) {//auto length
				if (d) {
				    if (cnmemonic[ADR_ZP]!=____ && val>=dpage && val<(dpage+0x100)) {val-=dpage;w=0;}
				    else if (cnmemonic[ADR_ADDR]!=____ && databank==(((unsigned)val) >> 16)) w=1;
				    else {
					w=val_length(val);
					if (w<2) w=2;
				    }
				} else {fixeddig=0;w=1;}
			    }
			    opr=ADR_ZP-w;ln=w+1; // lda $ff lda $ffff lda $ffffff
			}
                        lpoint--;
		    }// 13+2 Db
		}
		else if (c==2) {
		    if ((wht=what(&prm))==WHAT_SZ) {
			if ((wht=what(&prm))!=WHAT_Y) {err_msg(ERROR_GENERL_SYNTAX,NULL); break;}
			if (d) {
			    if (w==3) w=val_length(val);//auto length
			    if (w) w=3; // there's no lda ($ffffff,s),y or lda ($ffff,s),y!
			    opr=ADR_ZP_S_I_Y;
			} else fixeddig=0;
			ln=1; // lda ($ff,s),y
		    } // 16 Db
		    else {
			if (wht!=WHAT_XZ) {err_msg(ERROR_GENERL_SYNTAX,NULL); break;}
			if (cnmemonic[ADR_ADDR_X_I]==0x7C || cnmemonic[ADR_ADDR_X_I]==0xFC) {// jmp ($ffff,x) jsr ($ffff,x)
			    if (d) {
				if (w==3) w=1; else
				    if (w!=1) w=3; // there's no jmp ($ffffff,x)!
				opr=ADR_ADDR_X_I;
			    } else fixeddig=0;
			    ln=2; // jmp ($ffff,x)
			}
			else {
			    if (d) {
				val-=dpage;
				if (w==3) w=val_length(val);//auto length
				if (w) w=3; // there's no lda ($ffff,x) lda ($ffffff,x)!
				opr=ADR_ZP_X_I;
			    } else fixeddig=0;
			    ln=1; // lda ($ff,x)
			}
		    } // 18 Db
		}
		else {
		    if ((wht=what(&prm))==WHAT_Y) {
			if (d) {
			    val-=dpage;
			    if (w==3) w=val_length(val);
			    if (w) w=3;
			    opr=ADR_ZP_I_Y;
			} else fixeddig=0;
			ln=1; // lda ($ff),y
		    } // 19 Db
		    else if (wht==WHAT_EOL) {
			if (cnmemonic[ADR_ADDR_I]==0x6C) {// jmp ($ffff)
			    if (d) {
				if (fixeddig) {
				    if (w==3) {
					w=val_length(val);//auto length
					if (!w) w=1;
				    }
				    if (w!=1) w=3; // there's no jmp ($ffffff)!
				    if (!scpumode && (val & 0xff)==0xff) err_msg(ERROR______JUMP_BUG,NULL);//jmp ($xxff)
				} else w=1;
				opr=ADR_ADDR_I;
			    } else fixeddig=0;
			    ln=2; // jmp ($ffff)
			}
			else {
			    if (d) {
				val-=dpage;
				if (w==3) w=val_length(val);//auto length
				if (w) w=3; // there's no lda ($ffff) lda ($ffffff)!
				opr=ADR_ZP_I;
			    } else fixeddig=0;
			    ln=1; // lda ($ff)
			}
                        lpoint--;
		    } // 21 Db
		}
	    }
	    if (here()) {extrachar:err_msg(ERROR_EXTRA_CHAR_OL,NULL); break;}

	    if (d) {
		if (w==3) {err_msg(ERROR_CONSTNT_LARGE,NULL); break;}
		if ((cod=cnmemonic[opr])==____) {err_msg(ERROR_ILLEGAL_OPERA,NULL);break;}
	    }
	    {
		long temp=val;
		pokeb(cod ^ longbranch);
		switch (ln)
		{
		case 4:pokeb((unsigned char)temp);temp>>=8;
		case 3:pokeb((unsigned char)temp);temp>>=8;
		case 2:pokeb((unsigned char)temp);temp>>=8;
		case 1:pokeb((unsigned char)temp);
		}
	    }

	    if (listing) {
		long temp=val;
		int i;

		if (lastl!=1) {fputc('\n',flist);lastl=1;}
		fprintf(flist,".%06lx  %02x ",address-ln-1,(unsigned char)(cod ^ longbranch));

		for (i=0;i<4;i++)
		    if (ln>i) {fprintf(flist,"%02x ",(unsigned char)temp);temp>>=8;}
		    else fprintf(flist,"   ");

		if (arguments.monitor) {
		    fprintf(flist,"%c%c%c ",mnemonic[mnem*3],mnemonic[mnem*3+1],mnemonic[mnem*3+2]);
		    switch (opr) {
		    case ADR_IMPLIED: fprintf(flist,"         "); break;
		    case ADR_ACCU: fprintf(flist,"a        "); break;
		    case ADR_IMMEDIATE:
			{
			    if (ln==1) fprintf(flist,"#$%02x     ",(unsigned char)val);
			    else fprintf(flist,"#$%04x   ",(unsigned)(val&0xffff));
			    break;
			}
		    case ADR_LONG: fprintf(flist,"$%06x  ",(unsigned)(val&0xffffff)); break;
		    case ADR_ADDR: fprintf(flist,"$%04x    ",(unsigned)(val&0xffff)); break;
		    case ADR_ZP: fprintf(flist,"$%02x      ",(unsigned char)val); break;
		    case ADR_LONG_X: fprintf(flist,"$%06x,x",(unsigned)(val&0xffffff)); break;
		    case ADR_ADDR_X: fprintf(flist,"$%04x,x  ",(unsigned)(val&0xffff)); break;
		    case ADR_ZP_X: fprintf(flist,"$%02x,x    ",(unsigned char)val); break;
		    case ADR_ADDR_X_I: fprintf(flist,"($%04x,x)",(unsigned)(val&0xffff)); break;
		    case ADR_ZP_X_I: fprintf(flist,"($%02x,x)  ",(unsigned char)val); break;
		    case ADR_ZP_S: fprintf(flist,"$%02x,s    ",(unsigned char)val); break;
		    case ADR_ZP_S_I_Y: fprintf(flist,"($%02x,s),y",(unsigned char)val); break;
		    case ADR_ADDR_Y: fprintf(flist,"$%04x,y  ",(unsigned)(val&0xffff)); break;
		    case ADR_ZP_Y: fprintf(flist,"$%02x,y    ",(unsigned char)val); break;
		    case ADR_ZP_LI_Y: fprintf(flist,"[$%02x],y  ",(unsigned char)val); break;
		    case ADR_ZP_I_Y: fprintf(flist,"($%02x),y  ",(unsigned char)val); break;
		    case ADR_ADDR_LI: fprintf(flist,"[$%04x]  ",(unsigned)(val&0xffff)); break;
		    case ADR_ZP_LI: fprintf(flist,"[$%02x]    ",(unsigned char)val); break;
		    case ADR_ADDR_I: fprintf(flist,"($%04x)  ",(unsigned)(val&0xffff)); break;
		    case ADR_ZP_I: fprintf(flist,"($%02x)    ",(unsigned char)val); break;
		    case ADR_REL:
			if (ln==1) fprintf(flist,"$%06x  ",(unsigned)((((signed char)val)+l_address)&0xffffff));
                        else fprintf(flist,"$%06x  ",(unsigned)(((val >> 16)&0xffff)+(l_address&0xff0000)));
			break;
		    case ADR_REL_L: fprintf(flist,"$%06x  ",(unsigned)((((signed short)val)+l_address)&0xffffff)); break;
		    case ADR_MOVE: fprintf(flist,"$%02x,$%02x  ",(unsigned char)val,(unsigned char)(val>>8));
		    }
		}
		if (arguments.source) fprintf(flist," %s",llist); else fputc('\n',flist);
	    }
	    break;
	}
	default: if (skipit[waitforp]) err_msg(ERROR_GENERL_SYNTAX,NULL); //skip things if needed
	}
    }
end:
    if (oldpos==-1) closefile(fin); else fseek(fin,oldpos,SEEK_SET);
    cname[0]=0;
}

int main(int argc,char *argv[]) {
    time_t t;

    FILE* fout;

    testarg(argc,argv);

    fprintf(stdout,"6502/65C02 Turbo Assembler Version 1.3  Copyright (c) 1997 Taboo Productions\n");
    fprintf(stdout,"6502/65C02 Turbo Assembler Version 1.35 ANSI C port by [BiGFooT/BReeZe^2000]\n");
    fprintf(stdout,"6502/65C02/65816 TASM      Version 1.41 Fixing by Soci/Singular 2001\n");
    fprintf(stdout,"64TASS comes with ABSOLUTELY NO WARRANTY; This is free software, and you\n");
    fprintf(stdout,"are welcome to redistribute it under certain conditions; See LICENSE!\n");
    all_mem=(arguments.cpumode==OPCODES_65816)?0xffffff:0xffff;
    switch (last_mnem=arguments.cpumode) {
    case OPCODES_6502: mnemonic=MNEMONIC6502; opcode=c6502; break;
    case OPCODES_65C02:mnemonic=MNEMONIC65C02;opcode=c65c02;break;
    case OPCODES_6502i:mnemonic=MNEMONIC6502i;opcode=c6502i;break;
    case OPCODES_65816:mnemonic=MNEMONIC65816;opcode=c65816;scpumode=1;
    }

    fprintf(stdout,"\nAssembling file:   %s\n",arguments.input);

    if (!(mem64=malloc(all_mem+1))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
    if (arguments.nonlinear) if (!(mmap=malloc((all_mem+1) >> 3))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
//    memset(mem64,0,all_mem+1);
    if (arguments.nonlinear) memset(mmap,0,(all_mem+1) >> 3);

    do {
	if (pass++>20) {fprintf(stderr,"Ooops! Too many passes...\n");exit(1);}
	address=l_address=databank=dpage=longaccu=longindex=0;low_mem=all_mem;top_mem=0;
	fixeddig=1;waitfor[waitforp=0]=0;skipit[0]=1;sname[0]=0;sline=0;procname[0]=0;
//	listing=1;flist=stderr;
	compile(arguments.input,0,0,0,"",0,NULL);
	if (errors) {status();return 1;}
    } while (!fixeddig);

    if (arguments.list) {
        listing=1;
	if (!(flist=fopen(arguments.list,"wt"))) err_msg(ERROR_CANT_DUMP_LST,arguments.list);
	fprintf(flist,"\n;6502/65C02/65816 Turbo Assembler listing file of \"%s\"\n",arguments.input);
	time(&t);
	fprintf(flist,";done on %s\n",ctime(&t));

        pass++;
	address=l_address=databank=dpage=longaccu=longindex=0;low_mem=all_mem;top_mem=0;
	fixeddig=1;waitfor[waitforp=0]=0;skipit[0]=1;sname[0]=0;sline=0;procname[0]=0;
	compile(arguments.input,0,0,0,"",0,NULL);

	fprintf(flist,"\n;--- end of code ---\n");
	fclose(flist);
    }

    if (arguments.label) labelprint();

    if (errors) {status();return 1;}

    if (low_mem<=top_mem) {
	if ((fout=fopen(arguments.output,"wb"))==NULL) err_msg(ERROR_CANT_WRTE_OBJ,arguments.output);
	if (arguments.nonlinear) {
	    unsigned long bl_adr=low_mem, bl_len;
	    while (bl_adr<=top_mem) {
		while (bl_adr<all_mem && !(mmap[(bl_adr)>>3] & (1<<((bl_adr) & 7)))) bl_adr++;
		bl_len=bl_adr;
		while (bl_len<all_mem && (mmap[(bl_len)>>3] & (1<<((bl_len) & 7)))) bl_len++;
		bl_len-=bl_adr;
		fwrite(&bl_len,2+scpumode,1,fout);
		fwrite(&bl_adr,2+scpumode,1,fout);
		if (fwrite(mem64+bl_adr,bl_len,1,fout)==0) err_msg(ERROR_CANT_WRTE_OBJ,arguments.output);
		bl_adr+=bl_len;
	    }
	    bl_len=0;
	    fwrite(&bl_len,2+scpumode,1,fout);
	}
	else {
	    if (!arguments.stripstart) fwrite(&low_mem,2+scpumode,1,fout);
	    if (fwrite(mem64+low_mem,top_mem-low_mem+1,1,fout)==0) err_msg(ERROR_CANT_WRTE_OBJ,arguments.output);
	}
	if (fclose(fout)) err_msg(ERROR_CANT_WRTE_OBJ,arguments.output);
	status();
	return 0;
    }
    return 0;
}
