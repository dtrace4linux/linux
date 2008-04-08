CC=gcc -g
COMMON = ../../common/ctf
CPPFLAGS += -I../common -I../../common/ctf -I../../uts/common/ \
	-I$(COMMON) -I. \
	-I../../linux -DCTF_OLD_VERSIONS
CPPFLAGS += -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64

COMMON_OBJS = \
	ctf_create.o \
	ctf_error.o \
	ctf_hash.o \
	ctf_labels.o \
	ctf_lookup.o \
	ctf_open.o \
	ctf_types.o \
	ctf_util.o

libctf.a: \
	libctf.a(ctf_lib.o) \
	libctf.a(ctf_subr.o) \
	\
	libctf.a(ctf_create.o) \
	libctf.a(ctf_error.o) \
	libctf.a(ctf_hash.o) \
	libctf.a(ctf_labels.o) \
	libctf.a(ctf_lookup.o) \
	libctf.a(ctf_open.o) \
	libctf.a(ctf_types.o) \
	libctf.a(ctf_util.o)

libctf.a(ctf_create.o): $(COMMON)/ctf_create.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) rv libctf.a ctf_create.o
	rm -f ctf_create.o

libctf.a(ctf_error.o): $(COMMON)/ctf_error.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) rv libctf.a ctf_error.o
	rm -f ctf_error.o

libctf.a(ctf_hash.o): $(COMMON)/ctf_hash.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) rv libctf.a ctf_hash.o
	rm -f ctf_hash.o

libctf.a(ctf_labels.o): $(COMMON)/ctf_labels.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) rv libctf.a ctf_labels.o
	rm -f ctf_labels.o

libctf.a(ctf_lookup.o): $(COMMON)/ctf_lookup.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) rv libctf.a ctf_lookup.o
	rm -f ctf_lookup.o

libctf.a(ctf_open.o): $(COMMON)/ctf_open.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) rv libctf.a ctf_open.o
	rm -f ctf_open.o

libctf.a(ctf_types.o): $(COMMON)/ctf_types.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) rv libctf.a ctf_types.o
	rm -f ctf_types.o

libctf.a(ctf_util.o): $(COMMON)/ctf_util.c
	$(CC) $(CPPFLAGS) -c $<
	$(AR) rv libctf.a ctf_util.o
	rm -f ctf_util.o

