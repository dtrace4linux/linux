#	$(DTRACE) -h -s simple_probes.d 
CC=gcc -I../../uts/common -fPIC
#BITS=-m64
CFLAGS=$(BITS) $(BUILD_BITS)
BINDIR=../../$(BUILD_DIR)
DTRACE=$(BINDIR)/dtrace
DTRACE_DRTI_O=$(BINDIR)/drti.o
DTRACE_LIB=$(BINDIR)/libdtrace.a

all:	$(BINDIR)/simple-c
	@/bin/true

$(BINDIR)/simple-c: $(BINDIR)/simple_probes.o $(BINDIR)/simple.o \
	$(BINDIR)/shlib.so $(DTRACE_DRTI_O) $(DTRACE_LIB)
	$(CC) -Wl,-rpath `pwd` -o $(BINDIR)/simple-c $(BITS) $(BINDIR)/simple_probes.o $(BINDIR)/simple.o $(BINDIR)/shlib.so -ldl

$(BINDIR)/simple_probes.o: $(BINDIR)/simple.o simple_probes.d $(DTRACE_DRTI_O) $(DTRACE_LIB)
	@. $(BINDIR)/config.sh ; \
	if [ "$$BUILD_i386" = 1 ]; then \
		BITS=32 ; \
	else \
		BITS=64 ; \
	fi ; \
	echo DTRACE_DRTI_O=$(DTRACE_DRTI_O) $(DTRACE) -G -$$BITS -s simple_probes.d $(BINDIR)/simple.o ; \
	DTRACE_DRTI_O=$(DTRACE_DRTI_O) $(DTRACE) -x nolibs -G -$$BITS -s simple_probes.d $(BINDIR)/simple.o
	mv simple_probes.o $(BINDIR)

$(BINDIR)/simple.o: simple.c
	$(CC) -c simple.c
	mv simple.o $(BINDIR)

$(BINDIR)/shlib.so: shlib.c
	$(CC) -c shlib.c
	mv shlib.o $(BINDIR)
	ld -G -o $(BINDIR)/shlib.so $(BINDIR)/shlib.o
clean:
	-rm -f simple *.o *.so
