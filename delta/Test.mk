# see License.txt for copyright and terms of use

DIRS := test0_delta test1_multidelta test2_multidelta
.SUFFIXES:
.PHONY: check clean

check:
	for D in $(DIRS); do $(MAKE) -C $$D $@ || exit 1; done

clean:
	for D in $(DIRS); do $(MAKE) -C $$D $@; done
