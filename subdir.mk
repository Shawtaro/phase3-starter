# This is a sample Makefile for Phase 3.
# You may change this, e.g. to add new test cases, but keep in mind we will compile
# your phase1.c using our own copy of this file.
#
#       make            (makes libphase3.a and all tests)
#        make phase3     ditto
#
#       make testN      (makes testN)
#       make testN.out  (runs testN and puts output in testN.out)
#       make tests      (makes all testN.out files, i.e. runs all tests)
#       make tests_bg   (runs all tests in the background)
#
#       make testN.v    (runs valgrind on testN and puts output in testN.v)
#       make valgrind   (makes all testN.v files, i.e. runs valgrind on all tests)
#
#       make clean      (removes all files created by this Makefile)

# sh is dash on lectura which breaks things
SHELL = bash 

ifndef PREFIX
        PREFIX = $(HOME)
endif

# Compute the phase from the current working directory.
ifndef PHASE
	PHASE = $(lastword $(subst /, ,$(CURDIR)))
endif

PHASE_UPPER = $(shell tr '[:lower:]' '[:upper:]' <<< $(PHASE))
VERSION = $($(PHASE_UPPER)_VERSION)

# Compile all C files in this directory.
SRCS = $(wildcard *.c)

# Tests are in the "tests" directory.
TESTS = $(patsubst %.c,%,$(wildcard tests/*.c))

# Change this if you want to change the arguments to valgrind.
VGFLAGS = --track-origins=yes --leak-check=full --max-stackframe=100000

# Change this if you need to link against additional libraries (probably not).
LIBS = -lusloss$(USLOSS_VERSION) \
	   -luser$(USLOSS_VERSION) \
	   -ldisk$(USLOSS_VERSION) \
	   -lphase1a-$(PHASE1A_VERSION) \
	   -lphase1b-$(PHASE1B_VERSION) \
	   -lphase1c-$(PHASE1C_VERSION) \
	   -lphase1d-$(PHASE1D_VERSION) \
	   -lphase2a-$(PHASE2A_VERSION) \
	   -lphase2b-$(PHASE2B_VERSION) \
	   -lphase2c-$(PHASE2C_VERSION) \
	   -lphase2d-$(PHASE2D_VERSION)

STUBS = ./p3/p3stubs.o

ifeq ($(PHASE), phase3b)
	LIBS += -lphase3a-$(PHASE3A_VERSION)
else ifeq ($(PHASE), phase3c)
	LIBS += -lphase3a-$(PHASE3A_VERSION)
	LIBS += -lphase3b-$(PHASE3B_VERSION)
	STUBS =
else ifeq ($(PHASE), phase3d)
	LIBS += -lphase3a-$(PHASE3A_VERSION)
	LIBS += -lphase3b-$(PHASE3B_VERSION)
	LIBS += -lphase3c-$(PHASE3C_VERSION)
	STUBS =
endif

LIBS += -l$(PHASE)-$(VERSION) 

# Change this if you want change which flags are passed to the C compiler.
CFLAGS += -Wall -g -std=gnu99 -Werror -DPHASE=$(PHASE_UPPER) -DVERSION=$(VERSION) -DDATE="`date`"
#CFLAGS += -DDEBUG

# You shouldn't need to change anything below here. 

TARGET = lib$(PHASE)-$(VERSION).a

INCLUDES = -I. -I.. -I$(PREFIX)/include

ifeq ($(shell uname),Darwin)
	DEFINES += -D_XOPEN_SOURCE
	OS = macOS
	CFLAGS += -Wno-int-to-void-pointer-cast -Wno-extra-tokens -Wno-unused-label -Wno-unused-function
else
	OS = Linux
	CFLAGS += -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-unused-but-set-variable -Wno-unused-function
	LDFLAGS += -static
endif

CFLAGS += -DOS=$(OS)

CC=gcc
LD=gcc
AR=ar    
CFLAGS += $(INCLUDES) $(DEFINES)
LDFLAGS += -L. -L$(PREFIX)/cs452/lib -L$(PREFIX)/lib 
COBJS = ${SRCS:.c=.o}
DEPS = ${COBJS:.o=.d}
TSRCS = {$TESTS:=.c}
TOBJS = ${TESTS:=.o}
TDEPS = ${TOBJS:.o=.d}
TOUTS = ${TESTS:=.out}
TVS = ${TESTS:=.v}

# The following is to deal with circular dependencies between the USLOSS and phase1
# libraries. Unfortunately the linkers handle this differently on the two OSes.

ifeq ($(OS), macOS)
	LIBFLAGS = -Wl,-all_load $(LIBS)
else
	LIBFLAGS = -Wl,--start-group $(LIBS) -Wl,--end-group
endif

%.d: %.c
	$(CC) -c $(CFLAGS) -MM -MF $@ $<

all: $(PHASE)

$(PHASE): $(TARGET) $(TESTS)


$(TARGET):  $(COBJS)
	$(AR) -r $@ $^

install: $(TARGET)
	mkdir -p ~/lib
	install $(TARGET) ~/lib

.NOTPARALLEL: tests
tests: $(TOUTS)

# Remove implicit rules so that "make phaseX" doesn't try to build it from phaseX.c or phaseX.o
% : %.c

% : %.o

%.out: %
	./$< 1> $@ 2>&1

$(TESTS):   %: $(TARGET) %.o $(STUBS)
	$(LD) $(LDFLAGS) -o $@ $@.o $(STUBS) $(LIBFLAGS)

clean:
	rm -f $(COBJS) $(TARGET) $(TOBJS) $(TESTS) $(DEPS) $(TDEPS) $(TVS) $(STUBS) *.out tests/*.out tests/*.err

%.d: %.c
	$(CC) -c $(CFLAGS) -MM -MF $@ $<

valgrind: $(TVS)

%.v: %
	valgrind $(VGFLAGS) ./$< 1> $@ 2>&1

./p3/p3stubsTest: $(STUBS) ./p3/p3stubsTest.o
	$(LD) $(LDFLAGS) -o $@ $^

-include $(DEPS) 
-include $(TDEPS)
