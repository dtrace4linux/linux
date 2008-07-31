CC=gcc -g
CPPFLAGS += -I../libproc/common \
	-I../common/ctf \
	-I../linux \
	-I../lib/libproc/common \
	-I../libctf \
	-I../uts/common \
	-DCTF_OLD_VERSIONS $(PTR32)
CPPFLAGS += -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64

LIB = ../build/liblinux.a

$(LIB): \
	$(LIB)(gmatch.o) \
	$(LIB)(proc.o) \
	$(LIB)(proc2.o) \
	$(LIB)(proc_names.o) \
	$(LIB)(rd.o) \
	$(LIB)(strlcpy.o) \


#	$(LIB)(Psymtab.o) \
#	$(LIB)(Putil.o) \

clean:
	-rm -f *.o *.a

