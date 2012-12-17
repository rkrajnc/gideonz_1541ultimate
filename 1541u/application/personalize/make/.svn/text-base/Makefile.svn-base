#
# Makefile for SPI/SD card project
#
# This Makefile requires GNU make
#
# --------------------------------------------------------------------------
MAKEFILE = Makefile

# Core information
SYS	 = none
CL   = cl65
CC   = cc65
AS   = ca65
LD   = ld65
CP   = cp
MAKEAPPL = ../../../../tools/makeappl
HEX2BIN  = ../../../../tools/hex2bin

# This one comes with VICE
#C1541  	= c1541

LINKCFG = linker_run.cfg

# --------------------------------------------------------------------------
# Directories

PROJROOT = ../../..
INTERMEDIATE = ./output
RESULT = ./result
FPGA_DIR = ../../../../fpga/result
FPGA_RES = $(FPGA_DIR)/work
#FPGA_RES = ../../../../sd_drive_cartridge/syn/cart
BOOTFPGADIR = $(PROJROOT)/application/boot/make/result
ROMS = $(PROJROOT)/application/ultimate/roms
ULTIMATE_MAKE = $(PROJROOT)/application/ultimate/make

# --------------------------------------------------------------------------
# Files to be merged with output

FPGA_BIN_250E = $(FPGA_DIR)/ultimate_1541_v2.bin
FPGA_BIN_500E = $(FPGA_DIR)/ultimate_1541_500e.bin

ULTIMATE_APPL = $(PROJROOT)/application/ultimate/make/result/appl.bin
CHARS    = ../../../../binaries/characters_asc.bin
BOOTFPGA = $(FPGA_DIR)/bootfpga.bin


# --------------------------------------------------------------------------
# Includable files
INC0 = ../inc
INC1 = $(PROJROOT)/data/inc
INC2 = $(PROJROOT)/filesystem/fat/inc
INC3 = $(PROJROOT)/io/inc
INC4 = $(PROJROOT)/storage/card/inc
INC5 = $(PROJROOT)/storage/ram/inc
INC6 = $(PROJROOT)/storage/flash/inc
INC7 = $(PROJROOT)/ui/inc
INC8 = $(PROJROOT)/debug/inc
INC9 = $(PROJROOT)/system/inc
INCA = $(PROJROOT)/filetype/d64/inc
INCB = $(PROJROOT)/filetype/t64/inc

INC = -I$(INC0) -I$(INC1) -I$(INC2) -I$(INC3) -I$(INC4) -I$(INC5) -I$(INC6) -I$(INC7) -I$(INC8) -I$(INC9) -I$(INCA) -I$(INCB)

# --------------------------------------------------------------------------
# Library files
CLIB = $(PROJROOT)/lib/cart.lib
CLIB_DIR = $(PROJROOT)/lib/libsrc

# --------------------------------------------------------------------------
# Object files
OBJS =  startup.o version.o uart.o ff.o fat.o diskio.o sd.o spi.o onewire.o \
		mem_tools.o atmel_flash.o data_tools.o screen.o c64_irq.o freezer.o \
		keyb.o soft_signal.o dump_hex.o main.o personalize.o cartridge.o iec_test.o \
		ff_bankgate.o config.o atmel.o buttons.o card_irq.o cs8900a.o ethernet.o
# timing.o

# --------------------------------------------------------------------------
# GARBAGE
REMOVE = *~ *.map *.o *.s *.lbl *.d64 *.bin* *.out *.prm *.mcs *.bit *.mem *.txt

# --------------------------------------------------------------------------
# Project structure (virtual search path)


VPATH +=	$(PROJROOT)/application/src/ \
			$(PROJROOT)/io/src/ \
			$(PROJROOT)/filesystem/fat/src/ \
			$(PROJROOT)/storage/card/src/ \
			$(PROJROOT)/storage/ram/src \
			$(PROJROOT)/storage/flash/src \
			$(PROJROOT)/debug/src/ \
			$(PROJROOT)/data/src/ \
			$(PROJROOT)/ui/src/ \
			$(PROJROOT)/system/src/ \
			$(PROJROOT)/filetype/d64/src/ \
			$(PROJROOT)/filetype/t64/src/ \
			$(PROJROOT)/application/personalize/make/output \
			$(PROJROOT)/application/personalize/make/result \
			$(PROJROOT)/application/personalize/src/ \
			$(PROJROOT)/application/boot/make/result \
			$(PROJROOT)/test/menu/src

# --------------------------------------------------------------------------
# Compiler options
CFLAGS = -j -Oir -r -T -g -t $(SYS) $(INC) -DDEVELOPMENT -DDUMP_BYTES=8 -DFLASH_APPL="\"appl.bin"\"
#CFLAGS = -j -Oir -r -T -g -t $(SYS) $(INC)
AFLAGS = $(INC) 
# --------------------------------------------------------------------------
# Compiling


%.o: %.asm
	@echo
	@echo $@
	@$(CP) $^ $(INTERMEDIATE)/$(basename $@).s
	@$(AS) $(AFLAGS) $(INTERMEDIATE)/$(basename $@).s


%.o: %.c
	@echo
	@echo $@
	@$(CC) $(CFLAGS) $^ -o $(INTERMEDIATE)/$(basename $@).s
	@$(AS) $(AFLAGS) $(INTERMEDIATE)/$(basename $@).s

# --------------------------------------------------------------------------
# List of executables.

EXELIST	= prog.zip

# --------------------------------------------------------------------------
# Binaries

#.PRECIOUS:	fastcode.bin datspace.bin

.PHONY:	all
all:   	$(EXELIST)

$(CLIB)::
	@echo "Building Runtime Library..."
	@$(MAKE) -s -C $(CLIB_DIR)
	
$(FPGA_BIN_250E)::
	@echo "Calling FPGA make for 250E..."

#	@$(MAKE) -s -C $(FPGA_DIR)

$(FPGA_BIN_500E)::
	@echo "Calling FPGA make for 500E..."


code.bin fastcode.bin datspace.bin bank1.bin: $(OBJS) $(CLIB) $(MAKEFILE)
	@echo
	@echo $@
	@$(LD) -C $(LINKCFG) -vm -m $(INTERMEDIATE)/$(basename $@).map -o $(INTERMEDIATE)/$@ $(OBJS:%=$(INTERMEDIATE)/%) $(CLIB)
	@mv fastcode.bin $(INTERMEDIATE)/fastcode.bin
	@mv datspace.bin $(INTERMEDIATE)/datspace.bin
	@mv bank1.bin    $(INTERMEDIATE)/bank1.bin
	
prog.bin: code.bin fastcode.bin datspace.bin bank1.bin $(CHARS) $(FPGA_BIN_250E) $(MAKEFILE)
	@echo $@
	@echo "Done!"
	@$(MAKEAPPL) $(RESULT)/$@ $(FPGA_BIN_250E) 0 \
	    $(INTERMEDIATE)/bank1.bin 58000 \
	    $(INTERMEDIATE)/fastcode.bin 60000 \
	    $(INTERMEDIATE)/datspace.bin 6C000 \
	    $(CHARS) 70000 \
	    $(INTERMEDIATE)/code.bin 72000

bootfpga.bin: bootfpga.mcs
	@echo
	@echo $@
	@$(HEX2BIN) $(INTERMEDIATE)/$^ $(INTERMEDIATE)/$@


prog.zip: prog.bin
	@echo $@
	@echo "Calling Ultimate make..."
	@$(MAKE) -s -C $(ULTIMATE_MAKE) clean
	@export RELEASE=1
	@$(MAKE) -s -C $(ULTIMATE_MAKE)
	@export RELEASE=0
	@rm -rf $(INTERMEDIATE)/zip
	@rm -f $(RESULT)/$@
	@mkdir $(INTERMEDIATE)/zip
	@mkdir $(INTERMEDIATE)/zip/toflash
	@cp $(RESULT)/$^ $(INTERMEDIATE)/zip/prog.bin
	@cp $(ROMS)/*.bin $(INTERMEDIATE)/zip/toflash
	@cp $(BOOTFPGA) $(INTERMEDIATE)/zip/toflash
	@cp $(ULTIMATE_APPL) $(INTERMEDIATE)/zip/toflash
	@echo "Zipping results..."
	@chmod 755 $(INTERMEDIATE)/zip/toflash/*
	@cd $(INTERMEDIATE)/zip && zip -r -q ../../result/prog.zip *
	@echo "Done!"
    
# --------------------------------------------------------------------------
# Cleanup rules

.PHONY:	clean
clean:
	@echo Cleaning $(INTERMEDIATE)
	@cd $(INTERMEDIATE) && $(RM) $(REMOVE)
	@echo Cleaning $(RESULT)
	@cd $(RESULT) && $(RM) $(REMOVE)
#	@echo Cleaning FPGA
#	@cd $(FPGA_DIR) && $(RM) -r work

