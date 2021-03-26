CPPFLAGS := -O -g -Wall
CC = g++

MACRO11_SRCS = macro11.cpp \
	assemble.cpp assemble_globals.cpp assemble_aux.cpp	\
	extree.cpp listing.cpp macros.cpp parse.cpp rept_irpc.cpp symbols.cpp \
	mlb.cpp object.cpp stream2.cpp util.cpp rad50.cpp

MACRO11_OBJS = $(MACRO11_SRCS:.cpp=.o)

DUMPOBJ_SRCS = dumpobj.cpp rad50.cpp

DUMPOBJ_OBJS = $(DUMPOBJ_SRCS:.cpp=.o)

ALL_SRCS = $(MACRO11_SRCS) $(DUMPOBJ_SRCS)

all: macro11 dumpobj

tags: macro11 dumpobj
	ctags *.cpp *.h

macro11: $(MACRO11_OBJS) Makefile
	$(CC) $(CFLAGS) -o macro11 $(MACRO11_OBJS) -lm

dumpobj: $(DUMPOBJ_OBJS) Makefile
	$(CC) $(CFLAGS) -o dumpobj $(DUMPOBJ_OBJS)

MACRO11_OBJS: Makefile
DUMPOBJ_OBJS: Makefile

clean:
	-rm -f $(MACRO11_OBJS) $(DUMPOBJ_OBJS) macro11 dumpobj

macro11.o: macro11.cpp macro11.h rad50.h object.h  stream2.h \
 mlb.h util.h
mlb.o: mlb.cpp  rad50.h stream2.h mlb.h macro11.h util.h
object.o: object.cpp rad50.h object.h
stream2.o: stream2.cpp macro11.h  stream2.h
util.o: util.cpp util.h
rad50.o: rad50.cpp rad50.h
dumpobj.o: dumpobj.cpp rad50.h util.h
rad50.o: rad50.cpp rad50.h

# Since the only tests we have so far are for crashes,
# just try to assemble. Later, we will need expected/actual tests.

# Test that all options requiring a value bail out if it's not present.
argtests: macro11
	@ for OPT in -e -d -m -p -o -l -ysl ; do \
	  ./macro11 foo.mac $$OPT     2> /dev/null; \
    if (( $$? == 1 )); then echo PASS; else echo FAIL; fi; \
    echo "  $$OPT missing value"; \
	  ./macro11 foo.mac $$OPT -v  2> /dev/null; \
    if (( $$? == 1 )); then echo PASS; else echo FAIL; fi; \
    echo "  $$OPT fol. by option"; \
	done
	@ ./macro11 foo.mac $$OPT -x -v 2> /dev/null; \
    if (( $$? == 1 )); then echo PASS; else echo FAIL; fi; \
    echo "  -x must be the last option"

tests: macro11 argtests
	@ ACTUAL=`./macro11 tests/test-undef.mac 2>&1`; \
	if [ "tests/test-undef.mac:1: ***ERROR MACRO .TTYOU not found" == "$$ACTUAL" ]; then echo PASS; else echo FAIL; fi; \
	echo "  test-undef.mac"
