#
# Makefile for 1541 Ultimate Project
#
# This Makefile requires GNU make
#
# --------------------------------------------------------------------------
MAKEFILE = Makefile

# Core information
CC  = gcc



# --------------------------------------------------------------------------
# Compiling

%: %.c
	@echo
	@echo $@
	@$(CC) $(CFLAGS) $^ -o $(basename $@)

# --------------------------------------------------------------------------
# Binaries

.PHONY:	all
all:   	bin2hex hex2bin make_array make_mem makeappl promgen

	