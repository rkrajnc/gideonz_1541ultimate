#
# makefile for CC65 runtime library
#

.SUFFIXES: .o .obj .s .c

# Defines for executables. AR is used within this makefile, the others are
# passed to the submakes and contain paths relative to the subdirectories
# handled by the make subprocesses.
AR = ar65
AS = ca65
CC = cc65
LD = ld65


#-----------------------------------------------------------------------------
# SD Cartridge

.PHONY:	all
cartlib:
	@rm -f cart.lib
	@rm -f ../cart.lib
	@for i in common conio runtime cart ; do \
	    $(MAKE) -s SYS=none -C $$i || exit 1; \
	    $(AR) a cart.lib $$i/*.o; \
	done
	@mv cart.lib ..

#-----------------------------------------------------------------------------
# Dummy targets

.PHONY: clean
clean:
	rm -f cart.lib
	rm -f ../cart.lib
	@for i in cart conio common runtime $(ALLTARGETS); do\
       	   $(MAKE) -C $$i clean;			      		   	\
	done

.PHONY: zap
zap:	clean
	@for i in cart conio common runtime $(ALLTARGETS); do\
	   $(MAKE) -C $$i zap;     	    	       	      	   		\
	done
	@$(RM) *.o *.emd *.joy *.lib *.tgi *.ser *.mou



