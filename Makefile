# Build configuration
#VERBOSE=1
DEBUG=1

CROSS_COMPILE=

# Output Files
TARGET = nuclight

OLOC = ofiles

##########################################################################
# Check Host OS
##########################################################################

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CFLAGS += -DLINUX
	LINUX=1
endif
ifeq ($(UNAME_S),Darwin)
	CFLAGS += -DOSX
	OSX=1
endif


##########################################################################
# User configuration and firmware specific object files 
##########################################################################

# Overall system defines for compilation
ifdef DEBUG
	GCC_DEFINE= -DDEBUG
	DEBUG_OPTS = -ggdb3
	OPT_LEVEL = -Og
else
	GCC_DEFINE=
	DEBUG_OPTS =
	OPT_LEVEL = -O2
endif

# Directories for sources
App_DIR=Src
Inc_DIR=Inc
INCLUDE_PATHS = -I$(Inc_DIR)

GCC_DEFINE+= -std=gnu99

CFILES =
SFILES =
OLOC = ofiles

##########################################################################
# Project-specific files 
##########################################################################

# Main Files
# ==========

CFILES = $(App_DIR)/main.c $(App_DIR)/generics.c

##########################################################################
# Quietening
##########################################################################

ifdef VERBOSE
cmd = $1
Q :=
else
cmd = @$(if $(value 2),echo "$2";)$1
Q := @
endif

##########################################################################
# Compiler settings, parameters and flags
##########################################################################
GITTAG  = -DGIT_DESCRIBE=\"`git describe --tags --always --dirty`\"

LDFLAGS = 
CFLAGS +=  $(OPT_LEVEL) $(GITTAG) $(DEBUG_OPTS) -Wall $(LDFLAGS)

OBJS =  $(patsubst %.c,%.o,$(CFILES))
POBJS = $(patsubst %,$(OLOC)/%,$(OBJS))
PDEPS = $(POBJS:.o=.d)

##########################################################################
##########################################################################
##########################################################################

all: build 

$(OLOC)/%.o : %.c
	$(Q)mkdir -p $(basename $@)
	$(call cmd, \$(CC) -c $(CFLAGS) $(INCLUDE_PATHS) $(GCC_DEFINE) -MMD -o $@ $< ,\
	Compiling $<)

build: $(TARGET)

$(TARGET): $(POBJS)
	$(Q)$(CC) $(LDFLAGS) -o $(TARGET) $(MAP) $(POBJS)
	-@echo "Completed build of" $(TARGET)

clean:
	-$(call cmd, \rm -rf $(OLOC) $(TARGET) ,\
	Cleaning )

print-%:
	@echo $* is $($*)

pretty: clean
	@astyle --style=gnu -n --quiet --recursive --indent=spaces=2 --indent-classes --indent-switches --indent-preproc-block --indent-col1-comments --break-closing-brackets --add-brackets --convert-tabs --keep-one-line-statements --indent-cases --max-code-length=120 --break-after-logical --convert-tabs "$(Inc_DIR)/*.h" "$(App_DIR)/*.c"
	-@echo "Prettification complete"

-include $(PDEPS)
