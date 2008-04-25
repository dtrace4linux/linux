CC=gcc -g
CPPFLAGS += -I../common \
	-I../../../common/ctf \
	-I../../uts/common \
	-I../../linux \
	-I../../libproc/common \
	-I../../libctf/ \
	-DCTF_OLD_VERSIONS
CPPFLAGS += -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64
CPPFLAGS += -DYY_NO_INPUT

LIB = ../../build/libdtrace.a

all: dt_grammar.h $(LIB)

dt_grammar.h: dt_grammar.y

$(LIB): $(LIB)(dt_aggregate.o) \
	$(LIB)(dt_as.o) \
	$(LIB)(dt_cc.o) \
	$(LIB)(dt_cg.o) \
	$(LIB)(dt_consume.o) \
	$(LIB)(dt_decl.o) \
	$(LIB)(dt_dis.o) \
	$(LIB)(dt_dof.o) \
	$(LIB)(dt_error.o) \
	$(LIB)(dt_errtags.o) \
	$(LIB)(dt_grammar.o) \
	$(LIB)(dt_handle.o) \
	$(LIB)(dt_ident.o) \
	$(LIB)(dt_inttab.o) \
	$(LIB)(dt_lex.o) \
	$(LIB)(dt_link.o) \ \
	$(LIB)(dt_list.o) \
	$(LIB)(dt_module.o) \
	$(LIB)(dt_map.o) \
	$(LIB)(dt_open.o) \
	$(LIB)(dt_options.o) \
	$(LIB)(dt_parser.o) \
	$(LIB)(dt_pid.o) \
	$(LIB)(dt_printf.o) \
	$(LIB)(dt_proc.o) \
	$(LIB)(dt_program.o) \
	$(LIB)(dt_provider.o) \
	$(LIB)(dt_regset.o) \
	$(LIB)(dt_string.o) \
	$(LIB)(dt_strtab.o) \
	$(LIB)(dt_subr.o) \
	$(LIB)(dt_work.o) \
	$(LIB)(dt_xlator.o)

#$(LIB)(dt_lex.o): dt_lex.l

dt_errtags.c: dt_errtags.h mkerrtags.sh
	mkerrtags.sh <dt_errtags.h > dt_errtags.c

dt_lex.c: dt_lex.l dt_grammar.h
	lex -l -t -v dt_lex.l > dt_lex.c

clean:
	-rm -f *.o *.a

