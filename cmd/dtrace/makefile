CFLAGS=-g -W
LIBELF=/usr/lib/libelf.so.1
BINDIR=../../build
LIBS = \
	$(BINDIR)/libctf.a \
	$(BINDIR)/liblinux.a \
	$(BINDIR)/libdtrace.a \
	$(BINDIR)/libproc.a

all:	dtrace

dtrace: dtrace.o $(LIBS)
	libelf=$(LIBELF) ; \
	if [ -f /usr/lib64/libelf.so.1 ]; then \
		libelf=/usr/lib64/libelf.so.1 ; \
	fi ; \
	$(CC) $(CFLAGS) -o dtrace \
		-L../../build \
		dtrace.o -ldtrace -lctf -llinux -lpthread $$libelf -ldl

dtrace.o: dtrace.c
	$(CC) $(CFLAGS) -c \
		-I../../uts/common \
		-I../../libctf \
		-I../../libdtrace/common \
		-I../../libproc/common \
		-I../../linux \
		dtrace.c

test:
	./dtrace -x nolibs -n BEGIN -e -S

clean:
	-rm -f *.o dtrace

