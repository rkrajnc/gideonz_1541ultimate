/*

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

#define _MISC_C_
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include "argp.h"
#include "misc.h"
#include "opcodes.h"

struct arguments_t arguments={1,0,0,0,NULL,"a.out",OPCODES_6502,NULL,NULL,1,1,0,0};

static void *label_tree=NULL;
static void *macro_tree=NULL; 
static void *file_tree1=NULL;
static void *file_tree2=NULL;

char tolower_tab[256] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
    0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
    0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
    0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
    0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
    0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
    0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
    0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
    0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
    0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
    0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
    0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff};

unsigned char whatis[256]={
    WHAT_EOL,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,WHAT_EXPRESSION,WHAT_EXPRESSION,WHAT_HASHMARK,WHAT_EXPRESSION,WHAT_EXPRESSION,0,0,WHAT_EXPRESSION,0,WHAT_STAR,WHAT_EXPRESSION,WHAT_COMA,WHAT_EXPRESSION,WHAT_COMMAND,0,
    WHAT_EXPRESSION,WHAT_EXPRESSION,WHAT_EXPRESSION,WHAT_EXPRESSION,WHAT_EXPRESSION,WHAT_EXPRESSION,WHAT_EXPRESSION,WHAT_EXPRESSION,WHAT_EXPRESSION,WHAT_EXPRESSION,0,0,WHAT_EXPRESSION,WHAT_EQUAL,WHAT_EXPRESSION,0,
    WHAT_EXPRESSION,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,
    WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,0,0,0,0,WHAT_LBL,
    0,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,
    WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,WHAT_CHAR,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

char petascii(char ch) {
    if (!arguments.toascii) return ch;
    if ((ch>='a') && (ch<='z')) return ch-0x20;
    if ((ch>='A') && (ch<='Z')) return ch+0x80;
    return ch;
}

void err_msg(unsigned char no, char* prm) {
    char *warning[]={
	"Top of memory excedeed",
	"Possibly incorrectly used A",
	"Memory bank excedeed",
	"Possible jmp ($xxff) bug",
        "Long branch used"
    };
    char *error[]={
	"Double defined %s",
	"Not defined %s",
	"Extra characters on line",
	"Constant too large",
	"General syntax",
	"%s expected",
	"Expression syntax",
	"Branch too far",
	"Missing argument",
	"Illegal operand",
    };
    char *fatal[]={
	"Can't locate file: %s\n",
	"Out of memory\n",
	"Can't write object file: %s\n",
	"Line too long\n",
	"Can't write listing file: %s\n",
	"Can't write label file: %s\n",
	"%s\n",
	"File recursion\n"
    };

    fprintf(stderr,"%s:%ld: ",sname,sline);

    if (cname[0]) fprintf(stderr,"(%s:%ld:) ",cname,cline);

    if (no<0x40) {
	if (arguments.warning) {
	    fprintf(stderr,"warning: %s",warning[no]);
	    warnings++;
	}
    }
    else if (no<0x80) {
	if (no==ERROR____PAGE_ERROR)
	    fprintf(stderr,"Page error at $%06lx",l_address);
	else fprintf(stderr,error[no & 63],prm);
	errors++;
    }
    else {
	fprintf(stderr,"[**Fatal**] ");
	fprintf(stderr,fatal[no & 63],prm);
        fputc('\n',stderr);
        errors++;
	status();exit(1);
    }
    fprintf(stderr," \"%s\"\n",pline);

    if (errors==100) {fprintf(stderr,"Too many errors\n"); status(); exit(1);}
}

//----------------------------------------------------------------------
struct ize {char *name;};
void freetree(void *a)
{
    free(((struct ize *)a)->name);
    free(a);
}

void freemacrotree(void *a)
{
    free(((struct smacro *)a)->name);
    free(((struct smacro *)a)->file);
    free(a);
}

void freefiletree(void *a)
{
    if (((struct sfile *)a)->f) fclose(((struct sfile *)a)->f);
}

int label_compare(const void *aa,const void *bb)
{
    return strcmp(((struct ize *)aa)->name,((struct ize *)bb)->name);
}

int file_compare(const void *aa,const void *bb)
{
    return ((long)((struct sfile *)aa)->f)-((long)((struct sfile *)bb)->f);
}

struct slabel* find_label(char* name) {
    struct slabel **b;
    struct ize a;
    a.name=name;
    if (!(b=tfind(&a,&label_tree,label_compare))) return NULL;
    return *b;
}

// ---------------------------------------------------------------------------
struct slabel* lastlb=NULL;
struct slabel* new_label(char* name) {
    struct slabel **b;
    if (!lastlb)
	if (!(lastlb=malloc(sizeof(struct slabel)))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
    lastlb->name=name;
    if (!(b=tsearch(lastlb,&label_tree,label_compare))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
    if (*b==lastlb) { //new label
	if (!(lastlb->name=malloc(strlen(name)+1))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
        strcpy(lastlb->name,name);
	labelexists=0;lastlb=NULL;
    }
    else labelexists=1;
    return *b;            //already exists
}

// ---------------------------------------------------------------------------

struct smacro* find_macro(char* name) {
    struct smacro **b;
    struct ize a;
    a.name=name;
    if (!(b=tfind(&a,&macro_tree,label_compare))) return NULL;
    return *b;
}

// ---------------------------------------------------------------------------
struct smacro* lastma=NULL;
struct smacro* new_macro(char* name) {
    struct smacro **b;
    if (!lastma)
	if (!(lastma=malloc(sizeof(struct smacro)))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
    lastma->name=name;
    if (!(b=tsearch(lastma,&macro_tree,label_compare))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
    if (*b==lastma) { //new label
	if (!(lastma->name=malloc(strlen(name)+1))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
        strcpy(lastma->name,name);
	if (!(lastma->file=malloc(strlen(sname)+1))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
        strcpy(lastma->file,sname);
	labelexists=0;lastma=NULL;
    }
    else labelexists=1;
    return *b;            //already exists
}

// ---------------------------------------------------------------------------

struct sfile* lastfi=NULL;
FILE* openfile(char* name,char* volt) {
    struct sfile **b;
    if (!lastfi)
	if (!(lastfi=malloc(sizeof(struct sfile)))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
    lastfi->name=name;
    if (!(b=tsearch(lastfi,&file_tree1,label_compare))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
    if (*b==lastfi) { //new label
	if (!(lastfi->name=malloc(strlen(name)+1))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
        strcpy(lastfi->name,name);
	lastfi->f=fopen(name,"rt");
	if (!(b=tsearch(lastfi,&file_tree2,file_compare))) err_msg(ERROR_OUT_OF_MEMORY,NULL);
	lastfi=NULL;
        *volt=0;
    } else *volt=(*b)->open;
    (*b)->open=1;
    return (*b)->f;
}


void closefile(FILE* f) {
    struct sfile **b,a;
    a.f=f;
    if (!(b=tfind(&a,&file_tree2,file_compare))) return;
    (*b)->open=0;
}

void tfree() {
    tdestroy(label_tree,freetree);
    tdestroy(macro_tree,freemacrotree);
    tdestroy(file_tree1,freefiletree);
    tdestroy(file_tree2,freetree);
}

FILE *flab;
void kiir(const void *aa,VISIT value,int level)
{
    long val;
    if (value==leaf || value==preorder) {
#ifdef VICE
	fprintf(flab,"al %04lx .l%s\n",(*(struct slabel **)aa)->value,(*(struct slabel **)aa)->name);
#else
        if (!(*(struct slabel **)aa)->used) fputc(';',flab);
	fprintf(flab,"%-16s= ",(*(struct slabel **)aa)->name);
	if ((val=(*(struct slabel **)aa)->value)<0) fprintf(flab,"-");
	val=(val>=0?val:-val);
	if (val<0x100) fprintf(flab,"$%02lx\n",val);
	else if (val<0x10000l) fprintf(flab,"$%04lx\n",val);
	else if (val<0x1000000l) fprintf(flab,"$%06lx\n",val);
	else fprintf(flab,"$%08lx\n",val);
#endif
    }
}

void labelprint() {
    if (arguments.label) {
	if (!(flab=fopen(arguments.label,"wt"))) err_msg(ERROR_CANT_DUMP_LBL,arguments.label);
        twalk(label_tree,kiir);
	fclose(flab);
    }
}
//------------------------------------------------------------------
const char *argp_program_version="6502/65C02/65816 TASM 1.41";
const char *argp_program_bug_address="<soci@c64.rulez.org>";
const char doc[]="64tass Turbo Assembler Macro";
const char args_doc[]="SOURCE";

const struct argp_option options[]={
    {"no-warn"	, 	'w',		0,     	0,  "Suppress warnings"},
    {"nonlinear",	'n',		0,     	0,  "Generate nonlinear output file"},
    {"nostart" 	,	'b',		0,     	0,  "Strip starting address"},
    {"ascii" 	,	'a',		0,     	0,  "Convert ASCII to PETASCII"},
    {"case-sensitive",	'C',		0,     	0,  "Case sensitive labels"},
    {		0,	'o',"<file>"	,      	0,  "Place output into <file>"},
    {		0,	'D',"<label>=<value>",     	0,  "Define <label> to <value>"},
    {"long-branch",	'B',		0,     	0,  "Automatic bxx *+3 jmp $xxxx"},
    {		0,  	0,		0,     	0,  "Target selection:"},
    {"m65xx"  	,     	1,		0,     	0,  "Standard 65xx (default)"},
    {"m6502"  	,     	'i',		0,     	0,  "NMOS 65xx"},
    {"m65c02"  	,     	'c',		0,     	0,  "CMOS 65C02"},
    {"m65816"  	,     	'x',		0,     	0,  "W65C816"},
    {		0,  	0,		0,     	0,  "Source listing:"},
    {"labels"	,	'l',"<file>"	,      	0,  "List labels into <file>"},
    {"list"	,	'L',"<file>"	,      	0,  "List into <file>"},
    {"no-monitor",	'm',		0,      0,  "Don't put monitor code into listing"},
    {"no-source",	's',		0,      0,  "Don't put source code into listing"},
    {		0,  	0,		0,     	0,  "Misc:"},
    { 0 } 
};

static error_t parse_opt (int key,char *arg,struct argp_state *state)
{
    switch (key)
    {
    case 'w':arguments.warning=0;break;
    case 'n':arguments.nonlinear=1;break;
    case 'b':arguments.stripstart=1;break;
    case 'a':arguments.toascii=1;break;
    case 'o':arguments.output=arg;break;
    case 'D':
    {
	struct slabel* tmp;
	int i=0;
	while (arg[i] && arg[i]!='=') {
            if (!arguments.casesensitive) arg[i]=lowcase(arg[i]);
	    i++;
	}
	if (arg[i]=='=') {
            arg[i]=0;
	    tmp=new_label(arg);tmp->proclabel=0;
	    tmp->ertelmes=1;tmp->value=atoi(&arg[i+1]);
	}
	break;
    }
    case 'B':arguments.longbranch=1;break;
    case 1:arguments.cpumode=OPCODES_6502;break;
    case 'i':arguments.cpumode=OPCODES_6502i;break;
    case 'c':arguments.cpumode=OPCODES_65C02;break;
    case 'x':arguments.cpumode=OPCODES_65816;break;
    case 'l':arguments.label=arg;break;
    case 'L':arguments.list=arg;break;
    case 'm':arguments.monitor=0;break;
    case 's':arguments.source=0;break;
    case 'C':arguments.casesensitive=1;break;
    case ARGP_KEY_ARG:if (state->arg_num) argp_usage(state);arguments.input=arg;break;
    case ARGP_KEY_END:if (!state->arg_num) argp_usage(state);break;
    default:return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

const struct argp argp={options,parse_opt,args_doc,doc};

void testarg(int argc,char *argv[]) {
    argp_parse(&argp,argc,argv,0,0,&arguments);
}
