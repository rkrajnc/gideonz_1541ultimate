#
# Makefile for 1541 Ultimate Project
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
SDROM     = ../../../../tools/make_mem
MAKEAPPL  = ../../../../tools/makeappl
HEX2BIN   = ../../../../tools/hex2bin
BIN2HEX   = ../../../../tools/bin2hex
MYPROMGEN = ../../../../tools/promgen

#PROMGEN  = promgen
#DATA2MEM = data2mem

LINKCFG  = linker_run.cfg

# --------------------------------------------------------------------------
# Directories

ROMS     = ../roms
PROJROOT = ../../..
INTERMEDIATE = ./output
RESULT = ./result
FPGA_DIR = ../../../../fpga/result

# --------------------------------------------------------------------------
# Files to be merged with output

FPGA_BIN_250E = $(FPGA_DIR)/ultimate_1541_v2.bin
FPGA_BIN_500E = $(FPGA_DIR)/ultimate_1541_500e.bin

#ROM1541  = ../../../../binaries/1541.rom
#SAMPLES  = ../../../../binaries/drive_sounds.bin
CHARS    = ../../../../binaries/characters_asc.bin

# This one comes with VICE, currently not used
C1541  	= c1541


# --------------------------------------------------------------------------
# Includable files
INC0 = ../inc
INC1 = $(PROJROOT)/data/inc
INC2 = $(PROJROOT)/filesystem/fat/inc
INC3 = $(PROJROOT)/io/inc
INC4 = $(PROJROOT)/storage/ram/inc
INC5 = $(PROJROOT)/storage/flash/inc
INC6 = $(PROJROOT)/ui/inc
INC7 = $(PROJROOT)/debug/inc
INC8 = $(PROJROOT)/system/inc
INC9 = $(PROJROOT)/filetype/d64/inc
INCA = $(PROJROOT)/filetype/t64/inc
INCB = $(PROJROOT)/filetype/g64/inc
INCC = $(PROJROOT)/filetype/tap/inc
INCD = $(PROJROOT)/filetype/sid/inc

INC = -I$(INC0) -I$(INC1) -I$(INC2) -I$(INC3) -I$(INC4) -I$(INC5) -I$(INC6) -I$(INC7) -I$(INC8) -I$(INC9) -I$(INCA) -I$(INCB) -I$(INCC) -I$(INCD)

# --------------------------------------------------------------------------
# Library files
CLIB = $(PROJROOT)/lib/cart.lib
CLIB_DIR = $(PROJROOT)/lib/libsrc

# --------------------------------------------------------------------------
# Object files
OBJS =  startup.o uart.o ff.o fat.o diskio.o sd.o spi.o gcr.o main_loop.o onewire.o g64.o tap.o \
		mem_tools.o freezer.o atmel_flash.o data_tools.o screen.o c64_irq.o c64_menu.o copper.o \
		dir.o keyb.o soft_signal.o dma.o d64.o t64.o dump_hex.o bam_hdr.o main.o combsort.o last.o \
		asciipetscii.o iec.o  debug_console.o ff_bankgate.o d64_bankgate.o t64_bankgate.o sid.o \
		dir_bankgate.o config.o config_ui.o atmel.o buttons.o card_irq.o cartridge.o version.o \
		sid_play.o
		
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
			$(PROJROOT)/filetype/g64/ \
			$(PROJROOT)/filetype/t64/src/ \
			$(PROJROOT)/filetype/tap/src/ \
			$(PROJROOT)/filetype/sid/src/ \
			$(PROJROOT)/application/ultimate/make/output \
			$(PROJROOT)/application/ultimate/make/result \
			$(PROJROOT)/application/ultimate/src/ \
			$(PROJROOT)/test/menu/src \
			$(FPGA_DIR)
	 

# --------------------------------------------------------------------------
# Compiler options
ifeq ($(MAKELEVEL),0)
    export CFLAGS = -j -Oir -r -T -g -t $(SYS) $(INC) -DDEVELOPMENT
else
    export CFLAGS = -j -Oir -r -T -g -t $(SYS) $(INC)
endif

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
# Binaries

#.PRECIOUS:	fastcode.bin datspace.bin

.PHONY:	all
all:   	appl.bin appl_500.bin

$(CLIB)::
	@echo "Building Runtime Library..."
	@$(MAKE) -s -C $(CLIB_DIR)
	
#$(FPGA_DIR)/$(FPGANAME).bit::
#	@echo "Calling FPGA make..."
#	@$(MAKE) -s -C $(FPGA_DIR)

code.bin fastcode.bin datspace.bin bank1.bin: $(OBJS) $(CLIB) $(MAKEFILE)
	@echo
	@echo $@
	@$(LD) -C $(LINKCFG) -vm -m $(INTERMEDIATE)/$(basename $@).map -o $(INTERMEDIATE)/$@ $(OBJS:%=$(INTERMEDIATE)/%) $(CLIB)
	@mv fastcode.bin $(INTERMEDIATE)/fastcode.bin
	@mv datspace.bin $(INTERMEDIATE)/datspace.bin
	@mv bank1.bin    $(INTERMEDIATE)/bank1.bin
	
appl.bin: code.bin fastcode.bin datspace.bin bank1.bin $(CHARS) $(FPGA_BIN_250E) $(MAKEFILE)
	@echo $@
	@echo "Done!"
	@$(MAKEAPPL) $(RESULT)/$@ $(FPGA_BIN_250E) 0 \
	    $(INTERMEDIATE)/bank1.bin 58000 \
	    $(INTERMEDIATE)/fastcode.bin 60000 \
	    $(INTERMEDIATE)/datspace.bin 6C000 \
	    $(CHARS) 70000 \
	    $(INTERMEDIATE)/code.bin 72000

appl_500.bin: code.bin fastcode.bin datspace.bin bank1.bin $(CHARS) $(FPGA_BIN_500E) $(MAKEFILE)
	@echo $@
	@echo "Done!"
	@$(MAKEAPPL) $(RESULT)/$@ $(FPGA_BIN_500E) 0 \
	    $(INTERMEDIATE)/bank1.bin 58000 \
	    $(INTERMEDIATE)/fastcode.bin 60000 \
	    $(INTERMEDIATE)/datspace.bin 6C000 \
	    $(CHARS) 70000 \
	    $(INTERMEDIATE)/code.bin 72000

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

# 1010000-1017FFF  Action Replay ROM (32K)
# 1018000-101FFFF  Action Replay ROM (32K) - Alternate
# 1020000-102FFFF  Final Cartrige III (64K)
# 1030000-103xxxx  Audio samples (max 32K)
# 1038000-103BFFF  SID PLayer (max 16K)
# 103C000-103FFFF  1541 ROM (16K)
# 1040000-104FFFF  Retro Replay (64K)
# 1050000-105FFFF  Super Snapshot V5 (64K)
# 1060000-106FFFF  NTSC AR
# 1068000-106BFFF  1541C ROM
# 106C000-106FFFF  1541-ii ROM
# 1070000-107FFFF  NTSC Retro Replay
# 1080000-108FFFF  NTSC SuperSnapshot V5.22
# 1090000-109FFFF  PAL TAR w/ codenet PAL
# 10A0000-10AFFFF  PAL TAR w/ codenet NTSC
# 10B0000-10B1FFF  Epyx Fastloader
.PHONY: hex
hex:
	@echo Making Hex File
	@$(BIN2HEX) -o 0x010000 -t    $(ROMS)/ar5pal.bin   $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x018000 -t -a $(ROMS)/ar6pal.bin   $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x020000 -t -a $(ROMS)/final3.bin   $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x030000 -t    $(ROMS)/sounds.bin   $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x038000 -t -a $(ROMS)/sidcrt.bin   $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x03C000 -t -a $(ROMS)/1541.bin     $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x040000 -t -a $(ROMS)/rr38pal.bin  $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x050000 -t -a $(ROMS)/ss5pal.bin   $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x060000 -t -a $(ROMS)/ar5ntsc.bin  $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x068000 -t -a $(ROMS)/1541C.bin    $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x06C000 -t -a $(ROMS)/1541-ii.bin  $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x070000 -t -a $(ROMS)/rr38ntsc.bin $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x080000 -t -a $(ROMS)/ss5ntsc.bin  $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x090000 -t -a $(ROMS)/tar_pal.bin  $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x0A0000 -t -a $(ROMS)/tar_ntsc.bin $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x0B0000 -t -a $(ROMS)/epyx.bin     $(RESULT)/flash.hex
	@$(BIN2HEX) -o 0x140000 -t -a $(RESULT)/appl.bin   $(RESULT)/flash.hex
	@cat $(BOOT) >>$(RESULT)/flash.hex
	@$(HEX2BIN) -s 0x200000 $(RESULT)/flash.hex $(RESULT)/flash.bin
	@echo Done!
	