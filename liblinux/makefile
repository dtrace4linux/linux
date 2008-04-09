CC=gcc -g
CPPFLAGS += -I../libproc/common -I../../common/ctf -I../../uts/common/ \
	-I../../linux -I../lib/libproc/common -I../libctf -DCTF_OLD_VERSIONS
CPPFLAGS += -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64

LIB = liblinux.a

$(LIB): \
	$(LIB)(gmatch.o) \
	$(LIB)(proc.o) \
	$(LIB)(proc_names.o) \
	$(LIB)(strlcpy.o) \


#	$(LIB)(Psymtab.o) \
#	$(LIB)(Putil.o) \

