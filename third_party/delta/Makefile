# see License.txt for copyright and terms of use

REPOSITORY := http://delta.tigris.org/svn/delta/trunk
VERSION := 2006.08.03
DIR := delta-$(VERSION)

.SUFFIXES:
.PHONY: all check clean dist distclean

all:
	$(MAKE) -f Build.mk all

check:
	$(MAKE) -f Test.mk check

clean:
	$(MAKE) -f Build.mk clean
	$(MAKE) -f Test.mk clean

dist:
	svn export $(REPOSITORY) $(DIR)
	$(MAKE) -C $(DIR) -f Build.mk distbuild
	tar cvzf $(DIR).tar.gz $(DIR)
	chmod 444 $(DIR).tar.gz

distclean: clean
	rm -rf $(DIR)
	rm -f *.tar.gz
	$(MAKE) -f Build.mk distclean
