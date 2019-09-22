CC=gcc -g $(BUILD_BITS)
COMMON = ../common/ctf
CPPFLAGS += -I../common -I../common/ctf -I../uts/common/ \
	-I$(COMMON) -I. \
	-I../linux -DCTF_OLD_VERSIONS $(PTR32)
CPPFLAGS += -D_LARGEFILE_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64
BINDIR = ../$(BUILD_DIR)
ARFLAGS=Urv 

COMMON_OBJS = \
	ctf_create.o \
	ctf_error.o \
	ctf_hash.o \
	ctf_labels.o \
	ctf_lookup.o \
	ctf_open.o \
	ctf_types.o \
	ctf_uncompress.o \
	ctf_util.o

all: $(BINDIR)/libctf.a
	@/bin/true

$(BINDIR)/libctf.a: \
	$(BINDIR)/libctf.a(ctf_lib.o) \
	$(BINDIR)/libctf.a(ctf_subr.o) \
	\
	$(BINDIR)/libctf.a(ctf_create.o) \
	$(BINDIR)/libctf.a(ctf_error.o) \
	$(BINDIR)/libctf.a(ctf_hash.o) \
	$(BINDIR)/libctf.a(ctf_labels.o) \
	$(BINDIR)/libctf.a(ctf_lookup.o) \
	$(BINDIR)/libctf.a(ctf_open.o) \
	$(BINDIR)/libctf.a(ctf_types.o) \
	$(BINDIR)/libctf.a(uncompress.o) \
	$(BINDIR)/libctf.a(ctf_util.o)

$(BINDIR)/libctf.a(ctf_create.o): $(COMMON)/ctf_create.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) $(ARFLAGS) $(BINDIR)/libctf.a ctf_create.o
	rm -f ctf_create.o

$(BINDIR)/libctf.a(ctf_error.o): $(COMMON)/ctf_error.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) $(ARFLAGS) $(BINDIR)/libctf.a ctf_error.o
	rm -f ctf_error.o

$(BINDIR)/libctf.a(ctf_hash.o): $(COMMON)/ctf_hash.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) $(ARFLAGS) $(BINDIR)/libctf.a ctf_hash.o
	rm -f ctf_hash.o

$(BINDIR)/libctf.a(ctf_labels.o): $(COMMON)/ctf_labels.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) $(ARFLAGS) $(BINDIR)/libctf.a ctf_labels.o
	rm -f ctf_labels.o

$(BINDIR)/libctf.a(ctf_lookup.o): $(COMMON)/ctf_lookup.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) $(ARFLAGS) $(BINDIR)/libctf.a ctf_lookup.o
	rm -f ctf_lookup.o

$(BINDIR)/libctf.a(ctf_open.o): $(COMMON)/ctf_open.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) $(ARFLAGS) $(BINDIR)/libctf.a ctf_open.o
	rm -f ctf_open.o

$(BINDIR)/libctf.a(ctf_types.o): $(COMMON)/ctf_types.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) $(ARFLAGS) $(BINDIR)/libctf.a ctf_types.o
	rm -f ctf_types.o

$(BINDIR)/libctf.a(uncompress.o): uncompress.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) $(ARFLAGS) $(BINDIR)/libctf.a uncompress.o
	rm -f uncompress.o

$(BINDIR)/libctf.a(ctf_util.o): $(COMMON)/ctf_util.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) $(ARFLAGS) $(BINDIR)/libctf.a ctf_util.o
	rm -f ctf_util.o

clean:
	-rm -f *.o *.a
