# makefile for disx4

# version number for zipfile name
VERSION=4.3.0
DATE=$(shell date +%Y-%m-%d)

# get svn info
# if not running a Unix-like operating system you may have to comment the next line:
REV=$(shell ./svnrev)
#REV=r123

OBJS := disx.o disscrn.o disstore.o disline.o dissave.o discpu.o discmt.o rle.o \
        disz80.o dis6502.o dis6809.o dis68HC11.o dis68HC12.o dis68HC05.o \
        dis68k.o dis8051.o dis8048.o dis8008.o dis4004.o disz8.o dis1802.o \
        disf8.o dispic.o disarm.o disthumb.o dis9900.o disnova.o dis1610.o \
        dis7810.o dis78K0.o dis78K3.o dis8086.o dispdp11.o dis2650.o \
        disimp16.o \
        


all: disx
disx: $(OBJS)
run: all
	./disx


# C compiler target architecture
#   this is the setting for OS X to create a universal PPC/Intel binary
#   comment it out if you aren't running OS X with a multi-architecture compiler
#TARGET_ARCH = -arch ppc -arch i686


# Attempt to use pkg-config to get info for ncurses
# Use defaults if it is not found
ifeq (, $(shell which pkg-config))
  PKGLIBS   = -lncurses
  PKGFLAGS  =
else
  PKGLIBS  := $(shell pkg-config --libs   ncurses)
  PKGFLAGS := $(shell pkg-config --cflags ncurses)
endif


# C preprocessor flags for both C and C++
CPPFLAGS  = -g3 -O2 -Wall -Werror -Wextra -Wno-sign-compare
CPPFLAGS += -Wno-deprecated-declarations
CPPFLAGS += -DVERSION=\"$(VERSION)${REV}\" -DDATE=\"$(DATE)\"
CPPFLAGS += $(PKGFLAGS)
# C compiler flags
CFLAGS    = -std=c99
# C++ compiler flags
CXXFLAGS  = -fno-rtti -fno-exceptions

# libraries
LDLIBS  = $(PKGLIBS)
LDFLAGS = -lstdc++


.PHONY: zip
zip: all # build everything first to check for errors
	mkdir -p zip
	zip -r zip/disx4-$(DATE).zip *.cpp *.c *.h disx*.txt Makefile \
	equates svnrev


.PHONY: clean
clean:
	rm -f $(OBJS) disx


# "make depend" causes a dependency list of .h files to be created as
# the file ".depend". (Note that a file beginning with a period is normally
# considered to be invisible on Unix-based systems.)
# This file is then included for make to use later. If there is a new .cpp file
# or a change in the .h files used, this should be re-run.
depend: 
	# rm -f .depend
	touch .depend
	echo "*** please ignore the warnings about standard include files ***"
	makedepend -Y -f .depend -- $(CFLAGS) -- *.cpp *.h

# load the dependencies
# note that the "-" at the start means to silently ignore if the file is not found
-include .depend

# DO NOT DELETE
