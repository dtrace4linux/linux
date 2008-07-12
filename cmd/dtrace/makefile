CFLAGS=-g -W $(PTR32)
LIBELF=/usr/lib/libelf.so.1
BINDIR=../../build
LIBS = \
	$(BINDIR)/libctf.a \
	$(BINDIR)/libdtrace.a \
	$(BINDIR)/libproc.a \
	$(BINDIR)/liblinux.a

COMPILE = $(CC) $(CFLAGS) -o $(BINDIR)/dtrace \
		-L../../build \
		$(BINDIR)/dtrace.o -ldtrace -lctf -lproc -llinux -lz -lrt -lpthread $$libelf -ldl

all:	$(BINDIR)/dtrace

$(BINDIR)/dtrace: $(BINDIR)/dtrace.o $(LIBS)
	@libelf=$(LIBELF) ; \
	if [ -f /usr/lib64/libelf.so.1 ]; then \
		libelf=/usr/lib64/libelf.so.1 ; \
	elif [ -f /usr/local/lib/libelf.a ]; then \
		libelf=/usr/local/lib/libelf.a ; \
	fi ; \
	echo $(COMPILE) ; \
	$(COMPILE)

$(BINDIR)/dtrace.o: dtrace.c
	$(CC) $(CFLAGS) -c \
		-I../../uts/common \
		-I../../libctf \
		-I../../libdtrace \
		-I../../libproc/common \
		-I../../linux \
		dtrace.c
	mv dtrace.o $(BINDIR)

test:
	./dtrace -x nolibs -n BEGIN -e -S

clean:
	-rm -f *.o dtrace

