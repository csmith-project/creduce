## -*- mode: Makefile-Automake -*-
##
## Copyright (c) 2012, 2015 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

dist_noinst_SCRIPTS = \
	run_tests \
	test0.bat \
	test0.sh \
	test1.sh \
	test2.sh \
	test3.sh \
	test4.sh \
	test5.sh \
	test6.sh \
	test7.sh

dist_noinst_DATA = \
	file1.c \
	file2.c \
	file3.c

###

# The following rules are for the convenience of people who want to run tests
# in a separate build tree.
#
all-local:
	for f in $(dist_noinst_SCRIPTS) $(dist_noinst_DATA); do \
	  test -e "$$f" || ln -s "$(srcdir)/$$f" "$$f"; \
	done

clean-local:
	for f in $(dist_noinst_SCRIPTS) $(dist_noinst_DATA); do \
	  test \! -L "$$f" || rm -f "$$f"; \
	done

###############################################################################

## End of file.
