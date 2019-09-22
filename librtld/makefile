CC=gcc -g $(BUILD_BITS)
CPPFLAGS += -I../libproc/common \
	-I../common/ctf \
	-I../linux \
	-I../lib/libproc/common \
	-I../libctf \
	-I../uts/common \
	-I. \
	-DCTF_OLD_VERSIONS $(PTR32)
CPPFLAGS += -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64

BINDIR = ../$(BUILD_DIR)
LIB = $(BINDIR)/libdtrace.a

all: $(LIB)
	@/bin/true

$(LIB): \
	$(LIB)(rd_elf.o) \
	$(LIB)(rtld_db.o)

$(LIB)(rd_elf32.o): rd_elf.c
	$(CC) $(CPPFLAGS) -c rd_elf.c
	mv rd_elf.o rd_elf32.o
	ar $(ARFLAGS) $(LIB) rd_elf32.o
	rm -f rd_elf32.o

$(LIB)(rd_elf64.o): rd_elf.c
	$(CC) $(CPPFLAGS) -D_ELF64 -c rd_elf.c
	mv rd_elf.o rd_elf64.o
	ar $(ARFLAGS) $(LIB) rd_elf64.o
	rm -f rd_elf64.o
clean:
	-rm -f *.o *.a

