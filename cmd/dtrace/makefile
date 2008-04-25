CFLAGS=-g
LIBELF=/usr/lib/libelf.so.1
BINDIR=../../build
dtrace: dtrace.o \
	$(BINDIR)/libctf.a \
	$(BINDIR)/liblinux.a \
	$(BINDIR)/libdtrace.a \
	$(BINDIR)/libproc.a
	$(CC) $(CFLAGS) -o dtrace \
		-L../../libctf \
		-L../../libdtrace/common \
		-L../../libproc/common \
		-L../../liblinux \
		dtrace.o -ldtrace -lctf -llinux -lpthread $(LIBELF) -ldl

dtrace.o: dtrace.c
	$(CC) $(CFLAGS) -c \
		-I../../uts/common \
		-I..//libctf \
		-I..//libdtrace/common \
		-I..//libproc/common \
		-I../../linux \
		dtrace.c

test:
	./dtrace -x nolibs -n BEGIN -e -S

clean:
	-rm -f *.o dtrace

