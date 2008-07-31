OBJDIR = ../build
CFLAGS = -g

all: $(OBJDIR)/access $(OBJDIR)/kmem $(OBJDIR)/mknod $(OBJDIR)/pread

$(OBJDIR)/access: access.c
	$(CC) $(CFLAGS) -o $@ $<
$(OBJDIR)/kmem: kmem.c
	$(CC) $(CFLAGS) -o $@ $<
$(OBJDIR)/mknod: mknod.c
	$(CC) $(CFLAGS) -o $@ $<
$(OBJDIR)/pread: pread.c
	$(CC) $(CFLAGS) -o $@ $<

