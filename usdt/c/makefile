CC=gcc -I../../uts/common
#BITS=-m64
CFLAGS=$(BITS)
DTRACE=../../build/dtrace
DTRACE_DRTI_O=../../build/drti.o

all:	../../build/simple-c

../../build/simple-c: simple_probes.o simple.o shlib.so $(DTRACE_DRTI_O)
	$(CC) -Wl,-rpath `pwd` -o ../../build/simple-c $(BITS) simple_probes.o simple.o shlib.so -ldl

simple_probes.o: simple.o simple_probes.d $(DTRACE_DRTI_O)
	if [ "$BUILD_i386" = 1 ]; then \
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
