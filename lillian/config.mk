##############################################################################
# Project Configuration
#

PROJECT := lillian
PROJECT_TYPE := synth

##############################################################################
# Sources
#

EURORACKDIR = ../eurorack
BRAIDSDIR   = $(EURORACKDIR)/braids
STMLIBDIR   = $(EURORACKDIR)/stmlib

# C sources
CSRC = header.c

# C++ sources
CXXSRC = unit.cc digital_oscillator.cc $(BRAIDSDIR)/resources.cc $(BRAIDSDIR)/quantizer.cc $(BRAIDSDIR)/macro_oscillator.cc $(BRAIDSDIR)/analog_oscillator.cc $(STMLIBDIR)/utils/random.cc

# List ASM source files here
ASMSRC = 

ASMXSRC = 

##############################################################################
# Include Paths
#

UINCDIR  = $(EURORACKDIR) $(STMLIBDIR) $(STMLIBDIR)/third_party/STM $(STMLIBDIR)/third_party/STM/STM32F10x_StdPeriph_Driver/inc/ $(STMLIBDIR)/third_party/STM/CMSIS/CM3_f10x

##############################################################################
# Library Paths
#

ULIBDIR = 

##############################################################################
# Libraries
#

ULIBS  = -lm
ULIBS += -lc

##############################################################################
# Macros
#

UDEFS = 

