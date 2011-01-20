CFLAGS=-g -W $(PTR32) $(BUILD_BITS)
LIBELF=/usr/lib/libelf.so.1
BINDIR=../../$(BUILD_DIR)
LIBS = \
	$(BINDIR)/libctf.a \
	$(BINDIR)/libdtrace.a \
	$(BINDIR)/libproc.a \
	$(BINDIR)/liblinux.a

COMPILE = $(CC) $(CFLAGS) -o $(BINDIR)/dtrace \
		-L$(BINDIR) $$LIB32 \
		$(BINDIR)/dtrace.o -ldtrace -lctf -lproc -llinux -lz -lrt -lpthread $$libelf -ldl

all:	$(BINDIR)/dtrace $(BINDIR)/dwarf
	@/bin/true

######################################################################
#   Bit  of  a  mess below - we want libelf, but some older distros  #
#   may  not have it or we are on 64b kernel wanting to cross build  #
#   or vice versa.						     #
######################################################################
$(BINDIR)/dtrace: $(BINDIR)/dtrace.o $(LIBS)
	@libelf=$(LIBELF) ; \
	if [ "x$(BUILD_BITS)" = "x-m32" ]; then \
		LIB32=-L/usr/lib32 ; \
	fi ; \
	if [ -f /usr/lib/libelf.so ]; then \
		libelf=-lelf ; \
	elif [ -f /usr/lib64/libelf.so.1 ]; then \
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

$(BINDIR)/dwarf: ../../driver/dwarf.c
	$(CC) -o $(BINDIR)/dwarf ../../driver/dwarf.c -lelf

test:
	./dtrace -x nolibs -n BEGIN -e -S

clean:
	-rm -f *.o dtrace

