CFLAGS=-g
dtrace: dtrace.o ../../lib/libctf/libctf.a \
	../../lib/liblinux/liblinux.a \
	../../lib/libdtrace/common/libdtrace.a \
	../../lib/libproc/common/libproc.a
	$(CC) $(CFLAGS) -o dtrace \
		-L../../lib/libctf \
		-L../../lib/libdtrace/common \
		-L../../lib/libproc/common \
		-L../../lib/liblinux \
		dtrace.o -ldtrace -lctf -llinux -lpthread -lelf -ldl

dtrace.o: dtrace.c
	$(CC) $(CFLAGS) -c \
		-I../../uts/common \
		-I../../lib/libctf \
		-I../../lib/libdtrace/common \
		-I../../lib/libproc/common \
		-I../../linux \
		dtrace.c

test:
	./dtrace -x nolibs -n BEGIN -e -S
