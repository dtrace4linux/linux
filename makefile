#! /bin/make

# Makefile for dtrace. Everything is put into the build/ dir
# (well, nearly). Kernel litters the source directory, but dtrace
# the userland binary is in build/

# Allow me to create tmp releases.
rel=`if [ -z $$REL ] ; then date +%Y%m%d ; else echo $$REL ; fi`
RELDIR=dtrace

######################################################################
#   Avoid UTF quoted strings from gcc.				     #
######################################################################
LANG=C
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

notice: .first-time
	echo rel=$(rel)
	@echo "make all        - build everything - auto-detect (32 or 64 bit)"
	@echo "make clean      - clean out *.o/*.a and binaries"
	@echo "make install    - install files into target locations"
	@echo "make release    - create a new tarball for distribution"
	@echo "make load       - install the driver"
	@echo "make unload     - remove the driver"
	@echo "make test       - run cmd/dtrace regression tests."

.first-time:
	cat doc/README.first
	touch .first-time
release:
	tools/mkrelease.pl $$REL

######################################################################
#   Non-real releases, for my benefit.				     #
######################################################################
beta:
	$(MAKE) RELDIR=beta release

all:
	BUILD_DIR=$(BUILD_DIR) tools/build.pl $(BUILD_DIR) $(UNAME_M)
cmds:
	BUILD_DIR=$(BUILD_DIR) tools/build.pl -make do_cmds $(BUILD_DIR) $(UNAME_M)

all0:	do_cmds kernel

do_cmds:
	cd tests ; $(MAKE) $(NOPWD)
	cd libctf ; $(MAKE) $(NOPWD)
	cd libdtrace ; $(MAKE) $(NOPWD)
	cd liblinux ; $(MAKE) $(NOPWD)
	cd libproc/common ; $(MAKE) $(NOPWD)
	cd librtld ; $(MAKE) $(NOPWD)
	cd cmd/dtrace ; $(MAKE) $(NOPWD)
	cd cmd/ctfconvert ; $(MAKE) $(NOPWD)
	cd cmd/instr ; $(MAKE) $(NOPWD)
	cd usdt/c ; $(MAKE) $(NOPWD)
kernel:
	tools/mkdriver.pl all
	tools/mkctf.sh

clean:
	rm -rf build*
	rm -f .dtrace.nobug .first-time .test.prompt
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
	mkdir -p "$(DESTDIR)"/usr/lib/dtrace/$$CPU_BITS ; \
	rm -f "$(DESTDIR)"/usr/lib/dtrace/types.d ; \
	mkdir -p "$(DESTDIR)"/usr/sbin/ ; \
	install -m 4755 -o root build/dtrace "$(DESTDIR)"/usr/sbin/dtrace ; \
	install -m 644 -o root build/drti.o "$(DESTDIR)"/usr/lib/dtrace/$$CPU_BITS/drti.o
	mkdir -p "$(DESTDIR)"/etc/ ; \
	if [ ! -f "$(DESTDIR)"/etc/dtrace.conf ] ; then \
		install -m 644 -o root etc/dtrace.conf "$(DESTDIR)"/etc/dtrace.conf ; \
	fi
	install -m 644 -o root build/linux*.ctf "$(DESTDIR)"/usr/lib/dtrace
	install -m 644 -o root etc/io.d "$(DESTDIR)"/usr/lib/dtrace
	install -m 644 -o root etc/sched.d "$(DESTDIR)"/usr/lib/dtrace
	install -m 644 -o root etc/unistd.d "$(DESTDIR)"/usr/lib/dtrace
	scripts/mkinstall.pl -o="$(DESTDIR)"/usr/lib/dtrace

newf:
	tar cvf /tmp/new.tar `find . -newer TIMESTAMP -type f | \
		grep -v /build | grep -v ./tags | \
		grep -v ./cmd/dtrace/dtrace$$ | \
		grep -v dt_grammar.h | \
		grep -v '\*.so' | \
		grep -v '\.o$$'`

test:
	tools/tests.pl 
#	tools/runtests.pl
testloop:
	while true ; do tools/tests.pl run </dev/null ; done

######################################################################
#   Load  the driver -- we chmod 666 til i work out how to make the  #
#   driver do it. Otherwise dtrace needs to be setuid-root.	     #
######################################################################
load:
	tools/load.pl
	 
unl unload:
	-$(SUDO) /sbin/rmmod dtracedrv

######################################################################
#   Validate compilation on the kernels on my system(s)		     #
#   'warn'  is a simple tool to color code errors/warnings and give  #
#   grand  totals.  If  anyone wants a copy, let me know and I will  #
#   put  out  the  source,  but I need to de-crispify it, before it  #
#   will run. Its a single C program.				     #
######################################################################
kernels:
	tools/make_kernels.pl

x:
	rdate delly ; \
	mkupdate -m delly ; \
	make all
	tools/load.pl
