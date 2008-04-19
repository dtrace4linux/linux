CFLAGS=-g
dtrace: dtrace.o \
	../../libctf/libctf.a \
	../../liblinux/liblinux.a \
	../../libdtrace/common/libdtrace.a \
	../../libproc/common/libproc.a
	$(CC) $(CFLAGS) -o dtrace \
		-L../../libctf \
		-L../../libdtrace/common \
		-L../../libproc/common \
		-L../../liblinux \
		dtrace.o -ldtrace -lctf -llinux -lpthread -lelf -ldl

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

