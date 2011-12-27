BINDIR=../$(BUILD_DIR)

######################################################################
#   Build  useful  test  binaries. We may be a 64b host but not 32b  #
#   dev  system.  We  dont  care  that  much  whether  we  get both  #
#   binaries. More for my benefit.				     #
######################################################################
all:
	case `uname -m` in \
	  x86_64) \
		$(CC) -m64 -g -o $(BINDIR)/sys64 syscalls.c ; \
		$(CC) -m32 -g -o $(BINDIR)/sys32 syscalls.c ; \
		;; \
	  i686) \
		$(CC) -m32 -g -o $(BINDIR)/sys32 syscalls.c ; \
		;; \
	esac

