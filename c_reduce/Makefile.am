## -*- mode: Makefile.am -*-
##
## Copyright (c) 2012 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

dist_bin_SCRIPTS = \
	c_reduce.pl

perllibdir = $(pkgdatadir)/perl
dist_perllib_DATA = \
	creduce_regexes.pm \
	creduce_utils.pm \
	pass_balanced.pm \
	pass_clang.pm \
	pass_crc.pm \
	pass_indent.pm \
	pass_ints.pm \
	pass_lines.pm \
	pass_peep.pm \
	pass_ternary.pm

###############################################################################

# Follow the recipe described in the Autoconf manual for inserting Autoconf
# variables into scripts.  Notably, do *not* use AC_CONFIG_FILES in the root
# `configure.ac' file to do the substitution.  See the Autoconf manual section
# about "Installation Directory Variables" for details.

edit = sed \
	-e 's|@perllibdir[@]|$(perllibdir)|g' \
	-e 's|@prefix[@]|$(prefix)|g'

c_reduce.pl: Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in > $@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

c_reduce.pl: $(srcdir)/c_reduce.pl.in

CLEANFILES = \
	c_reduce.pl

###

# The `c_reduce.pl' script knows to look in its own directory for modules.
# But if one uses a separate build tree, the script doesn't know to look in
# the source tree.  So: handle that case by making symbolic links.
#
all-local:
	for pm in $(dist_perllib_DATA); do \
	  test -e "$$pm" || ln -s "$(srcdir)/$$pm" "$$pm"; \
	done

clean-local:
	for pm in $(dist_perllib_DATA); do \
	  test -L "$$pm" && rm -f "$$pm"; \
	done

###############################################################################

## End of file.
