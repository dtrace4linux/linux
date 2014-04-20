#! /bin/make

HDRS = \
	alist.h \
	barrier.h \
	ctf_headers.h \
	ctfmerge.h \
	ctftools.h \
	fifo.h \
	hash.h \
	libdwarfext.h \
	list.h \
	linux.h \
	memory.h \
	strtab.h \
	symbol.h \
	traverse.h \
	utils.h

CC = gcc -g -I. -I../../ -I../../libctf -I../../common \
	-I../../uts/common -I../../linux -I/usr/include/libdwarf

BINDIR = ../../build
OBJDIR = ../../build/ctfconvert.obj
OBJ=	$(OBJDIR)/ctfconvert.o \
	$(OBJDIR)/alist.o \
	$(OBJDIR)/ctf.o \
	$(OBJDIR)/dwarf.o \
	$(OBJDIR)/hash.o \
	$(OBJDIR)/iidesc.o \
	$(OBJDIR)/input.o \
	$(OBJDIR)/list.o \
	$(OBJDIR)/memory.o \
	$(OBJDIR)/merge.o \
	$(OBJDIR)/output.o \
	$(OBJDIR)/st_bugs.o \
	$(OBJDIR)/st_parse.o \
	$(OBJDIR)/stabs.o \
	$(OBJDIR)/stack.o \
	$(OBJDIR)/strtab.o \
	$(OBJDIR)/symbol.o \
	$(OBJDIR)/tdata.o \
	$(OBJDIR)/traverse.o \
	$(OBJDIR)/util.o
LIBS = ../../build/libctf.a $(LIBDWARF) -lbfd -lelf -lz
# Older machines used this. We may need to autodetect which is correct.
#LIBS = ../../build/libctf.a $(LIBDWARF) -liberty -lelf -lz

######################################################################
#   Fedora core will use /usr/lib64/libdwarf.so. We might want, but  #
#   dont  care  about static libdwarf.a on FC14+. Ideally, we would  #
#   get   rid   of   this   dependency   because   it   complicates  #
#   cross-platform builds.					     #
######################################################################
all:
	@. ../../build/port.sh ; \
	if [ ! -f /usr/lib/libdwarf.a -a ! -f /usr/lib64/libdwarf.a ]; then \
		echo "=================================================================" ; \
		echo "=== You need /usr/lib/libdwarf.a or /usr/lib64/libdwarf.a " ; \
		echo "=== (libdw is not yet supported) to build." ; \
		echo "=== " ; \
		echo "=== apt-get install binutils-dev" ; \
		echo "=== apt-get install libdw-dev" ; \
		echo "=== " ; \
		echo "=== Without these, we will not build ctfconvert (needed for "; \
		echo "=== SDT structure definitions)." ; \
		echo "=================================================================" ; \
		exit 0 ; \
	else \
		mkdir -p $(OBJDIR) ; \
		$(MAKE) all1 ; \
	fi 

all1: $(BINDIR)/ctfconvert $(BINDIR)/ctfdump

$(BINDIR)/ctfconvert: $(OBJDIR) $(OBJ)
	$(CC) -o $(BINDIR)/ctfconvert $(OBJ) $(LIBS)

$(BINDIR)/ctfdump: $(OBJDIR)/compare.o $(OBJDIR)/dump.o $(OBJDIR)/utils.o
	$(CC) -o $(BINDIR)/ctfdump $(OBJDIR)/alist.o $(OBJDIR)/ctf.o $(OBJDIR)/dump.o \
		$(OBJDIR)/hash.o $(OBJDIR)/iidesc.o $(OBJDIR)/input.o \
		$(OBJDIR)/list.o $(OBJDIR)/memory.o $(OBJDIR)/merge.o $(OBJDIR)/output.o \
		$(OBJDIR)/st_bugs.o \
		$(OBJDIR)/st_parse.o \
		$(OBJDIR)/stabs.o $(OBJDIR)/stack.o $(OBJDIR)/strtab.o \
		$(OBJDIR)/symbol.o $(OBJDIR)/tdata.o $(OBJDIR)/traverse.o \
		$(OBJDIR)/utils.o $(OBJDIR)/util.o $(LIBS)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/alist.o: alist.c $(HDRS)
	$(CC) -c alist.c
	mv alist.o $(OBJDIR)

$(OBJDIR)/ctf.o: ctf.c $(HDRS)
	$(CC) -c ctf.c
	mv ctf.o $(OBJDIR)

$(OBJDIR)/ctfconvert.o: ctfconvert.c $(HDRS)
	$(CC) -c ctfconvert.c
	mv ctfconvert.o $(OBJDIR)

$(OBJDIR)/compare.o: compare.c $(HDRS)
	$(CC) -c compare.c
	mv compare.o $(OBJDIR)

$(OBJDIR)/dump.o: dump.c $(HDRS)
	$(CC) -c dump.c
	mv dump.o $(OBJDIR)

$(OBJDIR)/dwarf.o: dwarf.c $(HDRS) ../../build/port.h
	$(CC) -c dwarf.c
	mv dwarf.o $(OBJDIR)

$(OBJDIR)/hash.o: hash.c $(HDRS)
	$(CC) -c hash.c
	mv hash.o $(OBJDIR)

$(OBJDIR)/input.o: input.c $(HDRS)
	$(CC) -c input.c
	mv input.o $(OBJDIR)

$(OBJDIR)/iidesc.o: iidesc.c $(HDRS)
	$(CC) -c iidesc.c
	mv iidesc.o $(OBJDIR)

$(OBJDIR)/list.o: list.c $(HDRS)
	$(CC) -c list.c
	mv list.o $(OBJDIR)

$(OBJDIR)/memory.o: memory.c $(HDRS)
	$(CC) -c memory.c
	mv memory.o $(OBJDIR)

$(OBJDIR)/merge.o: merge.c $(HDRS)
	$(CC) -c merge.c
	mv merge.o $(OBJDIR)

$(OBJDIR)/output.o: output.c $(HDRS)
	$(CC) -c output.c
	mv output.o $(OBJDIR)

$(OBJDIR)/st_bugs.o: st_bugs.c $(HDRS)
	$(CC) -c st_bugs.c
	mv st_bugs.o $(OBJDIR)

$(OBJDIR)/st_parse.o: st_parse.c $(HDRS)
	$(CC) -c st_parse.c
	mv st_parse.o $(OBJDIR)

$(OBJDIR)/stabs.o: stabs.c $(HDRS)
	$(CC) -c stabs.c
	mv stabs.o $(OBJDIR)

$(OBJDIR)/stack.o: stack.c $(HDRS)
	$(CC) -c stack.c
	mv stack.o $(OBJDIR)

$(OBJDIR)/strtab.o: strtab.c $(HDRS)
	$(CC) -c strtab.c
	mv strtab.o $(OBJDIR)

$(OBJDIR)/symbol.o: symbol.c $(HDRS)
	$(CC) -c symbol.c
	mv symbol.o $(OBJDIR)

$(OBJDIR)/tdata.o: tdata.c $(HDRS)
	$(CC) -c tdata.c
	mv tdata.o $(OBJDIR)

$(OBJDIR)/traverse.o: traverse.c $(HDRS)
	$(CC) -c traverse.c
	mv traverse.o $(OBJDIR)

$(OBJDIR)/util.o: util.c $(HDRS)
	$(CC) -c util.c
	mv util.o $(OBJDIR)

$(OBJDIR)/utils.o: utils.c $(HDRS)
	$(CC) -c utils.c
	mv utils.o $(OBJDIR)

