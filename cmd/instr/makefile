CFLAGS=-g -W $(PTR32) $(BUILD_BITS)
BINDIR=../../$(BUILD_DIR)

all:	$(BINDIR)/instr 
	@/bin/true

######################################################################
#   Bit  of  a  mess below - we want libelf, but some older distros  #
#   may  not have it or we are on 64b kernel wanting to cross build  #
#   or vice versa.						     #
######################################################################
$(BINDIR)/instr: instr.c
	$(CC) -DUSERMODE -Dprintk=printf -DDIS_TEXT -I../../driver \
		-o $(BINDIR)/instr instr.c ../../driver/instr_size.c ../../driver/dis_tables.c
