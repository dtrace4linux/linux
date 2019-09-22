ARFLAGS=Urv 
CC=gcc -g $(BUILD_BITS)
CPPFLAGS += -I../libproc/common \
	-I../common/ctf \
	-I../linux \
	-I../lib/libproc/common \
	-I../libctf \
	-I../uts/common \
	-DCTF_OLD_VERSIONS $(PTR32)
CPPFLAGS += -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64

BINDIR = ../$(BUILD_DIR)
LIB = $(BINDIR)/liblinux.a

all: $(LIB)
	@/bin/true

$(LIB): \
	$(LIB)(cpu_status.o) \
	$(LIB)(getopt.o) \
	$(LIB)(gmatch.o) \
	$(LIB)(proc.o) \
	$(LIB)(proc2.o) \
	$(LIB)(proc_create.o) \
	$(LIB)(proc_names.o) \
	$(LIB)(proc_util.o) \
	$(LIB)(strlcpy.o)


#	$(LIB)(Psymtab.o) \
#	$(LIB)(Putil.o) \

clean:
	-rm -f *.o *.a

