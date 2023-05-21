##############################################################################
# Project Configuration
#

PROJECT := braids4dl
PROJECT_TYPE := synth

##############################################################################
# Sources
#

BRAIDSDIR = ../eurorack/braids
STMLIBDIR = ../eurorack/stmlib

# C sources
CSRC = header.c

# C++ sources
CXXSRC = unit.cc ../Maximilian/src/maximilian.cpp $(BRAIDSDIR)/digital_oscillator.cc $(BRAIDSDIR)/resources.cc $(BRAIDSDIR)/quantizer.cc $(BRAIDSDIR)/macro_oscillator.cc $(BRAIDSDIR)/analog_oscillator.cc $(STMLIBDIR)/utils/random.cc

# List ASM source files here
ASMSRC = 

ASMXSRC = 

##############################################################################
# Include Paths
#

UINCDIR  = ../Maximilian/src $(STMLIBDIR) $(STMLIBDIR)/third_party/STM $(STMLIBDIR)/third_party/STM/STM32F10x_StdPeriph_Driver/inc/ $(STMLIBDIR)/third_party/STM/CMSIS/CM3_f10x

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

