## -*- mode: Makefile-Automake -*-
##
## Copyright (c) 2012, 2013, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

# Yuck.  Is there a better way to distribute the sources for a script that is
# "compiled"?
#
nodist_bin_SCRIPTS = \
	creduce

perllibdir = $(pkgdatadir)/perl
dist_perllib_DATA = \
	creduce_regexes.pm \
	creduce_utils.pm \
	pass_balanced.pm \
	pass_blank.pm \
	pass_clang.pm \
	pass_clang_binsrch.pm \
	pass_clex.pm \
	pass_comments.pm \
	pass_include_includes.pm \
	pass_includes.pm \
	pass_indent.pm \
	pass_ints.pm \
	pass_lines.pm \
	pass_peep.pm \
	pass_special.pm \
	pass_ternary.pm \
	pass_unifdef.pm
nodist_perllib_DATA = \
	creduce_config.pm

EXTRA_DIST = \
	CMakeLists.txt \
	creduce.in \
	creduce_config.pm.in

###############################################################################

GIT_HASH := $(shell "$(top_srcdir)/git-hash.sh" "$(top_srcdir)" || echo error)

DUMMY1 := $(shell echo "$(GIT_HASH)" > git_version.txt.tmp)
DUMMY2 := $(shell cmp -s git_version.txt.tmp git_version.txt || \
		  cp git_version.txt.tmp git_version.txt)
DUMMY3 := $(shell rm -f git_version.txt.tmp)

# Follow the recipe described in the Autoconf manual for inserting Autoconf
# variables into scripts.  Notably, do *not* use AC_CONFIG_FILES in the root
# `configure.ac' file to do the substitution.  See the Autoconf manual section
# about "Installation Directory Variables" for details.

# Note: the `perl' substitution is special so that we can use it on the
# script's shebang line.  See `man perlrun' for details.
creduce_edit = sed \
	-e 's|@perl[@]|$(PERL)|g' \
	-e 's|@PERL[@]|$(PERL)|g' \
	-e 's|@perllibdir[@]|$(perllibdir)|g' \
	-e 's|@prefix[@]|$(prefix)|g'

creduce: Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(creduce_edit) $${srcdir}$@.in > $@.tmp
	chmod +x $@.tmp
	mv $@.tmp $@

creduce: creduce.in

###

creduce_config_edit = sed \
	-e 's|@bindir[@]|$(bindir)|g' \
	-e 's|@libexecdir[@]|$(libexecdir)|g' \
	-e 's|@PACKAGE_BUGREPORT[@]|$(PACKAGE_BUGREPORT)|g' \
	-e 's|@PACKAGE_NAME[@]|$(PACKAGE_NAME)|g' \
	-e 's|@PACKAGE_STRING[@]|$(PACKAGE_STRING)|g' \
	-e 's|@PACKAGE_URL[@]|$(PACKAGE_URL)|g' \
	-e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g' \
	-e 's|@VERSION[@]|$(VERSION)|g' \
	-e 's|@GIT_HASH[@]|$(GIT_HASH)|g' \
	-e 's|@CLANG_FORMAT[@]|$(CLANG_FORMAT)|g'

creduce_config.pm: Makefile git_version.txt
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(creduce_config_edit) $${srcdir}$@.in > $@.tmp
	mv $@.tmp $@

creduce_config.pm: creduce_config.pm.in

CLEANFILES = \
	creduce \
	creduce_config.pm \
	git_version.txt \
	git_version.txt.tmp

###

# The `creduce' script knows to look in its own directory for modules.  But if
# one uses a separate build tree, the script doesn't know to look in the source
# tree.  Handle that case by making symbolic links to the source tree.  (These
# symlinks do not impede the `install' target, fortunately.)
#
all-local:
	for pm in $(dist_perllib_DATA); do \
	  test -e "$$pm" || ln -s "$(srcdir)/$$pm" "$$pm"; \
	done

clean-local:
	for pm in $(dist_perllib_DATA); do \
	  test \! -L "$$pm" || rm -f "$$pm"; \
	done

###############################################################################

## End of file.
