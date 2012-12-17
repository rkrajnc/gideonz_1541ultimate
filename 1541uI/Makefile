#
# Makefile for 1541 Ultimate project
#
# This Makefile requires GNU make
#
# --------------------------------------------------------------------------
MAKEFILE = Makefile
CP = cp

# --------------------------------------------------------------------------
# Directories

TOOLS    = ./tools
ULTIMATE = ./1541u/application/ultimate/make
RESULT   = $(ULTIMATE)/result/appl.bin
RESULT2  = $(ULTIMATE)/result/appl_500.bin

# --------------------------------------------------------------------------
# List of executables.

EXELIST	= appl.bin

# --------------------------------------------------------------------------
# Binaries

#.PRECIOUS:	fastcode.bin datspace.bin

.PHONY:	all
all:   	$(EXELIST)

appl.bin::
	@echo "Calling Tools make..."
	@$(MAKE) -s -C $(TOOLS)
	@echo "Calling Ultimate make..."
	@$(MAKE) -s -C $(ULTIMATE) clean
	@$(MAKE) -s -C $(ULTIMATE)
	@$(CP) $(RESULT) .
	@$(CP) $(RESULT2) .
