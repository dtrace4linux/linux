CC=gcc -g $(BUILD_BITS)
CPPFLAGS += -I. \
	-I../../common/ctf \
	-I../uts/common \
	-I../linux \
	-I../libproc/common \
	-I../libctf/ \
	-DCTF_OLD_VERSIONS $(PTR32)
#CPPFLAGS += -DYY_NO_INPUT

BINDIR = ../$(BUILD_DIR)
CPPFLAGS += -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -I$(BINDIR)
LIB = $(BINDIR)/libdtrace.a
H = \
	./dt_as.h		\
	./dt_decl.h		\
	./dt_errtags.h		\
	./dt_ident.h		\
	./dt_impl.h		\
	./dt_inttab.h		\
	./dt_list.h		\
	./dt_linux.h		\
	./dt_module.h		\
	./dt_parser.h		\
	./dt_pid.h		\
	./dt_printf.h		\
	./dt_proc.h		\
	./dt_provider.h		\
	./dt_regset.h		\
	./dt_string.h		\
	./dt_strtab.h		\
	./dt_xlator.h		\
	./dtrace.h

all: $(LIB) $(BINDIR)/drti.o
	@/bin/true

$(LIB): \
	$(LIB)(dis_tables.o) \
	$(LIB)(dt_lex.o) \
	$(LIB)(dt_aggregate.o) \
	$(LIB)(dt_as.o) \
	$(LIB)(dt_buf.o) \
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
	$(LIB)(dt_isadep.o) \
	$(LIB)(dt_list.o) \
	$(LIB)(dt_link.o) \
	$(LIB)(dt_module.o) \
	$(LIB)(dt_map.o) \
	$(LIB)(dt_names.o) \
	$(LIB)(dt_open.o) \
	$(LIB)(dt_options.o) \
	$(LIB)(dt_parser.o) \
	$(LIB)(dt_pcb.o) \
	$(LIB)(dt_pid.o) \
	$(LIB)(dt_pragma.o) \
	$(LIB)(dt_print.o) \
	$(LIB)(dt_printf.o) \
	$(LIB)(dt_proc.o) \
	$(LIB)(dt_program.o) \
	$(LIB)(dt_provider.o) \
	$(LIB)(dt_regset.o) \
	$(LIB)(dt_string.o) \
	$(LIB)(dt_strtab.o) \
	$(LIB)(dt_subr.o) \
	$(LIB)(dt_work.o) \
	$(LIB)(dt_xlator.o) \
	$(LIB)(stubs.o)

dt_errtags.c: dt_errtags.h mkerrtags.sh
	./mkerrtags.sh <dt_errtags.h > dt_errtags.c || ( rm dt_errtags.c && exit 1 )
dt_names.c: mknames.sh
	./mknames.sh <../uts/common/sys/dtrace.h | sed -e 's/\\n/\n/g' > dt_names.c

$(BINDIR)/dt_grammar.h $(LIB)(dt_grammar.o): dt_grammar.y $(H) 
	../tools/yacc.pl -d dt_grammar.y
	mv y.tab.h $(BINDIR)/dt_grammar.h
	mv y.tab.c dt_grammar.c
	$(CC) -DYYDEBUG=1 -DYYERROR_VERBOSE $(CPPFLAGS) -c dt_grammar.c
	ar rv $(LIB) dt_grammar.o
	-rm -f dt_grammar.c dt_grammar.o

#dt_grammar.h: $(LIB)(dt_grammar.o)

$(LIB)(dt_lex.o): $(BINDIR)/dt_grammar.h $(BINDIR)/dt_lex.c $(H)
	$(CC) $(CPPFLAGS) -c $(BINDIR)/dt_lex.c
	ar rv $(LIB) dt_lex.o
	-rm -f dt_lex.o
$(BINDIR)/dt_lex.c: dt_lex.l
	lex -t -v dt_lex.l > $(BINDIR)/dt_lex.c

$(BINDIR)/drti.o: drti.c $(H)
	$(CC) -fPIC $(BUILD_BITS) $(CPPFLAGS) -c drti.c
	mv drti.o $(BINDIR)

$(LIB)(dis_tables.o): ../driver/dis_tables.c $(H)
	$(CC) $(CPPFLAGS) -Ii386 -I../driver -c ../driver/dis_tables.c
	ar rv $(LIB) dis_tables.o
	-rm -f dis_tables.o
$(LIB)(dt_names.o): dt_names.c
	$(CC) $(CPPFLAGS) -c dt_names.c
	ar rv $(LIB) dt_names.o
	-rm -f dt_names.o

$(LIB)(dt_isadep.o): i386/dt_isadep.c $(H)
	$(CC) $(CPPFLAGS) -Ii386 -I../driver -c i386/dt_isadep.c
	ar rv $(LIB) dt_isadep.o
	-rm -f dt_isadep.o
clean:
	-rm -f dt_errtags.c dt_names.c dt_grammar.h dt_lex.c *.o *.a

