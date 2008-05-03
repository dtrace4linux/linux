#! /bin/make

rel=`date +%Y%m%d`

MAKEFLAGS += --no-print-directory

# Use sudo if you want ...
SUDO=setuid root

######################################################################
#   List of drivers we have:					     #
#    								     #
#   dtrace: The grand daddy which marshalls the other components.    #
#    								     #
#   fbt  -  function  boundary  tracing - provider to intercept any  #
#   function							     #
######################################################################
DRIVERS = dtrace fasttrap fbt

notice:
	echo rel=$(rel)
	@echo "make all        - build everything (which builds!)"
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
	if [ ! -d build ] ; then \
		mkdir build ; \
	fi
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
	-$(SUDO) insmod drivers/fasttrap/fasttrap.ko
	-$(SUDO) insmod drivers/fbt/fbt.ko
	$(SUDO) chmod 666 /dev/dtrace
unl:
	-$(SUDO) rmmod fbt
	-$(SUDO) rmmod fasttrap
	-$(SUDO) rmmod dtracedrv

