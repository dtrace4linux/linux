#! /bin/make

# Makefile for dtrace. Everything is put into the build/ dir
# (well, nearly). Kernel litters the source directory, but dtrace
# the userland binary is in build/

# Allow me to create tmp releases.
rel=`if [ -z $$REL ] ; then date +%Y%m%d ; else echo $$REL ; fi`
RELDIR=dtrace

UNAME_M=`uname -m`
NOPWD = --no-print-directory
MAKEFLAGS += --no-print-directory

ifdef BUILD_KERNEL
	BUILD_DIR=build-$(BUILD_KERNEL)
else
	BUILD_DIR=build-$(shell uname -r)
endif

# Use sudo if you want ...
SUDO=setuid root

######################################################################
#   List of drivers we have:					     #
#    								     #
#   ctf - compact type framework.				     #
#    								     #
#   dtrace: The grand daddy which marshalls the other components.    #
#    								     #
#   fasttrap - trace user land applications.    		     #
#    								     #
#   fbt  -  function  boundary  tracing - provider to intercept any  #
#   function							     #
#    								     #
#   systrace - system call tracing.             		     #
######################################################################
DRIVERS = dtrace 

notice:
	echo rel=$(rel)
	@echo "make all        - build everything - auto-detect (32 or 64 bit)"
	@echo "make clean      - clean out *.o/*.a and binaries"
	@echo "make install    - install files into target locations"
	@echo "make release    - create a new tarball for distribution"
	@echo "make load       - install the driver"
	@echo "make unload     - remove the driver"
	@echo "make test       - run cmd/dtrace regression tests."

release:
	eval `grep build= .release` ; \
	build=`expr $$build + 1` ; \
	(echo date=`date` ; echo release=dtrace-$(rel) ; echo build=$$build ) > .release
	find . -name checksum.lst | xargs rm -f
	cd .. ; mv dtrace dtrace-$(rel) ; \
	tar cvf - --exclude=*.o \
		--exclude=.*.cmd \
		--exclude=*.so \
		--exclude=*.mod.c \
		--exclude=build \
		--exclude=build-* \
		--exclude=libdtrace/dt_grammar.h \
		--exclude=libdtrace/dt_lex.c \
		--exclude=.tmp_versions \
		--exclude=Module.symvers \
		--exclude=*.ko \
		--exclude=*.a \
		--exclude=*.mod \
		--exclude=tags \
		--exclude=lwn \
		--exclude=dt_lex.c \
		--exclude=.dtrace.nobug \
		dtrace-$(rel) | bzip2 >/tmp/dtrace-$(rel).tar.bz2 ; \
	mv dtrace-$(rel) dtrace
	rcp /tmp/dtrace-$(rel).tar.bz2 minny:release/website/$(RELDIR)
	ls -l /tmp/dtrace-$(rel).tar.bz2

######################################################################
#   Non-real releases, for my benefit.				     #
######################################################################
beta:
	$(MAKE) RELDIR=beta release

all:
	BUILD_DIR=$(BUILD_DIR) tools/build.pl $(BUILD_DIR) $(UNAME_M)

all0:
	cd libctf ; $(MAKE) $(NOPWD)
	cd libdtrace ; $(MAKE) $(NOPWD)
	cd liblinux ; $(MAKE) $(NOPWD)
	cd libproc/common ; $(MAKE) $(NOPWD)
	cd cmd/dtrace ; $(MAKE) $(NOPWD)
	cd usdt/c ; $(MAKE) $(NOPWD)
	tools/mkdriver.pl all

clean:
	rm -rf build?*
	rm -f usdt/*/*.o
	rm -f usdt/*/*.so
	cd libctf ; $(MAKE) clean
	cd libdtrace ; $(MAKE) clean
	cd liblinux ; $(MAKE) clean
	cd libproc/common ; $(MAKE) clean
	cd cmd/dtrace ; $(MAKE) clean
	tools/mkdriver.pl clean

install: build/dtrace build/config.sh
	. build/config.sh ; \
	mkdir -p /usr/lib/dtrace/$$CPU_BITS ; \
	install -m 4755 -o root build/dtrace /usr/sbin/dtrace ; \
	install -m 644 -o root build/drti.o /usr/lib/dtrace/$$CPU_BITS/drti.o

newf:
	tar cvf /tmp/new.tar `find . -newer TIMESTAMP -type f | \
		grep -v /build | grep -v ./tags | \
		grep -v ./cmd/dtrace/dtrace$$ | \
		grep -v dt_grammar.h | \
		grep -v '\*.so' | \
		grep -v '\.o$$'`

test:
	tools/runtests.pl

######################################################################
#   Load  the driver -- we chmod 666 til i work out how to make the  #
#   driver do it. Otherwise dtrace needs to be setuid-root.	     #
######################################################################
load:
	tools/load.pl
	 
unl unload:
	-$(SUDO) rmmod dtracedrv

######################################################################
#   Validate compilation on the kernels on my system(s)		     #
######################################################################
kernels:
	for i in /lib/modules/2* ; \
	do \
		dir=`basename $$i` ; \
		echo "======= Building: $$i ===============================" ; \
		BUILD_KERNEL=$$dir make all || exit 1 ; \
	done
