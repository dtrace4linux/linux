#! /bin/make

rel=`date +%Y%m%d`

MAKEFLAGS += --no-print-directory

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
	@echo "make release    - create a new tarball for distribution"

release:
	cd .. ; mv dtrace dtrace-$(rel) ; \
	tar cvf - --exclude=*.o \
		--exclude=.*.cmd \
		--exclude=*.mod.c \
		--exclude=dtrace/dtrace \
		--exclude=.tmp_versions \
		--exclude=Module.symvers \
		--exclude=*.ko \
		--exclude=*.a \
		--exclude=*.mod \
		--exclude=tags \
		--exclude=dt_lex.c \
		dtrace-$(rel) | bzip2 >/tmp/dtrace-$(rel).tar.bz2 ; \
	mv dtrace-$(rel) dtrace
	rcp /tmp/dtrace-$(rel).tar.bz2 minny:release/website/dtrace
	ls -l /tmp/dtrace-$(rel).tar.bz2

all:
	if [ `arch` != x86_64 ]; then \
	  	export PTR32=-D_ILP32 ; \
		export BUILD_i386=1 ; \
	fi ; \
	if [ ! -d build ] ; then \
		mkdir build ; \
	fi ; \
	$(MAKE) all0
all0:
	cd libctf ; $(MAKE) 
	cd libdtrace/common ; $(MAKE)
	cd liblinux ; $(MAKE)
	cd libproc/common ; $(MAKE)
	cd cmd/dtrace ; $(MAKE)
	for i in $(DRIVERS) ; \
	do  \
		echo "******** drivers/$$i" ; \
		cd drivers/$$i ; ../make-me ; \
		cd ../.. ; \
	done

clean:
	rm -f build/*
	cd libctf ; $(MAKE) clean
	cd libdtrace/common ; $(MAKE) clean
	cd liblinux ; $(MAKE) clean
	cd libproc/common ; $(MAKE) clean
	cd cmd/dtrace ; $(MAKE) clean
	for i in $(DRIVERS) ; \
	do \
		cd drivers/$$i ; make clean ; \
		cd ../.. ; \
	done

######################################################################
#   Load  the driver -- we chmod 666 til i work out how to make the  #
#   driver do it. Otherwise dtrace needs to be setuid-root.	     #
######################################################################
ins:
	sync ; sync
	-$(SUDO) insmod drivers/dtrace/dtracedrv.ko
	sleep 1
	$(SUDO) chmod 666 /dev/dtrace
	$(SUDO) chmod 666 /dev/fbt
	grep ' kallsyms' /proc/kallsyms >/dev/fbt
	grep ' get_symbol_offset' /proc/kallsyms >/dev/fbt
unl:
	-$(SUDO) rmmod dtracedrv
