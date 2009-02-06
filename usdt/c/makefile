#	$(DTRACE) -h -s simple_probes.d 
CC=gcc -I../../uts/common -fPIC
#BITS=-m64
CFLAGS=$(BITS)
BINDIR=../../$(BUILD_DIR)
DTRACE=$(BINDIR)/dtrace
DTRACE_DRTI_O=$(BINDIR)/drti.o

all:	$(BINDIR)/simple-c
	@/bin/true

$(BINDIR)/simple-c: simple_probes.o simple.o shlib.so $(DTRACE_DRTI_O)
	$(CC) -Wl,-rpath `pwd` -o $(BINDIR)/simple-c $(BITS) simple_probes.o simple.o shlib.so -ldl

simple_probes.o: simple.o simple_probes.d $(DTRACE_DRTI_O)
	. $(BINDIR)/config.sh ; \
	if [ "$$BUILD_i386" = 1 ]; then \
		BITS=32 ; \
	else \
		BITS=64 ; \
	fi ; \
	DTRACE_DRTI_O=$(DTRACE_DRTI_O) $(DTRACE) -G -$$BITS -s simple_probes.d simple.o

shlib.so: shlib.c
	$(CC) -c shlib.c
	ld -G -o shlib.so shlib.o
clean:
	-rm -f simple *.o *.so
