#
# makefile for CC65 runtime library
#

.SUFFIXES: .o .s .c

#--------------------------------------------------------------------------
# Programs and flags

SYS	= none

AS	= ca65
CC	= cc65
LD	= ld65

AFLAGS	= -t $(SYS) -I../../asminc
CFLAGS	= -Osir -g -T -t $(SYS) --forget-inc-paths -I . -I ../../include

#--------------------------------------------------------------------------
# Rules

%.o:   	%.c
	@$(CC) $(CFLAGS) $<
	@$(AS) -o $@ $(AFLAGS) $(*).s

%.o:	%.s
	@$(AS) -g -o $@ $(AFLAGS) $<

#--------------------------------------------------------------------------
# Object files

C_OBJS =  	

S_OBJS =	ctype.o	

#--------------------------------------------------------------------------
# Targets

.PHONY:	all clean zap

all:	$(C_OBJS) $(S_OBJS)

clean:
	@$(RM) *~ *.bck $(C_OBJS:.o=.s) $(C_OBJS) $(S_OBJS)

zap:	clean

