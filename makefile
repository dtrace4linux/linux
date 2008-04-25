#! /bin/make

rel=`date +%Y%m%d`

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
		--exclude=*.ko \
		--exclude=*.a \
		--exclude=tags \
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
	cd drivers/dtrace ; ./make-me

clean:
	rm -f build/*
	cd libctf ; $(MAKE) clean
	cd libdtrace/common ; $(MAKE) clean
	cd liblinux ; $(MAKE) clean
	cd libproc/common ; $(MAKE) clean
	cd cmd/dtrace ; $(MAKE) clean
	cd drivers/dtrace ; $(MAKE) clean

