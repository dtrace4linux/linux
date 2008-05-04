SUBDIRS=dtrace

all:
	for dir in $(SUBDIRS); do (cd $$dir; ../make-me ); done
clean:
	for dir in $(SUBDIRS); do (cd $$dir; $(MAKE) clean ); done

