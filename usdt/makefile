CC=gcc -I../uts/common
#BITS=-m64
CFLAGS=$(BITS)
DTRACE=dtrace

simple: simple_probes.o simple.o
	$(CC) -o simple $(BITS) simple_probes.o simple.o

simple_probes.o: simple.o simple_probes.d
	$(DTRACE) -G -64 -s simple_probes.d simple.o

clean:
	-rm -f *.o
