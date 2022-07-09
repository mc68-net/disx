# makefile for disx4

# version number for zipfile name
VERSION=4.1.0
DATE=$(shell date +%Y-%m-%d)

# get svn info
# if not running a Unix-like operating system you may have to comment the next line:
REV=$(shell ./svnrev)
#REV=r123

OBJS := disx.o disscrn.o disstore.o disline.o dissave.o discpu.o discmt.o rle.o \
        disz80.o dis6502.o dis68HC11.o dis68HC05.o dis6809.o dis68k.o dis8051.o \
        dis8048.o disz8.o dis1802.o disf8.o dispic.o disarm.o dis7810.o dis78K0.o \
        dis78K3.o


all: disx
disx: $(OBJS)
run: all
	./disx


# C compiler target architecture
#   this is the setting for OS X to create a universal PPC/Intel binary
#   comment it out if you aren't running OS X with a multi-architecture compiler
#TARGET_ARCH = -arch ppc -arch i686

# C compiler flags
CPPFLAGS = -g
CFLAGS = -Wall -Werror -Wextra -std=c99 -O2 -DVERSION=\"$(VERSION)${REV}\" -DDATE=\"$(DATE)\"
CXXFLAGS = -Wall -Werror -Wextra -Wno-sign-compare -fno-rtti -fno-exceptions -O2 -DVERSION=\"$(VERSION)${REV}\" -DDATE=\"$(DATE)\"

# libraries
LDLIBS = -lncurses


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
