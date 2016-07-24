## -*- mode: Makefile-Automake -*-
##
## Copyright (c) 2013, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

libexec_PROGRAMS = clex strlex

clex_CPPFLAGS =

clex_SOURCES = \
	clex.l \
	defs.h \
	driver.c

strlex_CPPFLAGS =

strlex_SOURCES = \
	strlex.l \
	defs.h \
	driver.c

EXTRA_DIST = \
	CMakeLists.txt

# SHORT STORY: Always rebuild `clex.c' from `clex.l' if Make thinks that
# `clex.c' is out of date.  This is achieved by a sinful Automake hack.
#
am__skiplex :=
#
# LONG STORY: We use AM_MAINTAINER_MODE in C-Reduce so as to avoid rebuilding
# our `configure' and `Makefile.in' files unnecessarily.  (The mode is disabled
# by default, meaning that `configure' avoids creating Makefile rules that
# perform various "package maintenance activities.")
#
# Automake handles lex-derived C files specially when AM_MAINTAINER_MODE is
# used.  It treats regeneration of the C file is a maintenance activity, not a
# normal compilation activity.  Treating lex as a maintenance activity is a
# pain in the ass for C-Reduce developers, because it means that they must do
# something more than `make' when they change `clex.l' and want to recompile.
#
# Automake short-circuits the `.l' -> `.c' compilation step by testing if the
# target `.c' file already exists.  This is implemented by the Make macro
# `am__skiplex', which is prefixed to the usual command line for running lex:
#
#   am__skiplex = test -f $@ ||
#
# (This macro is not defined when AM_MAINTAINER_MODE is not used, nor is it
# defined when maintainer mode is enabled by the user.  But I digress.)
#
# We don't want to treat running lex as a maintenance activity; we want to
# treat it as a normal compilation activity.  We achieve this by redefining the
# internal `am__skiplex' macro, thus avoiding the short-circuit described
# above.  This implementation is icky and sinful because `am__skiplex' is
# internal to Automake and undocumented.  It works with Automake 1.13.2.
#
# Treating lex/yacc as special makes some sense historically, when the builder
# of a package could not be assumed to have working, installed copies of those
# tools.  But I think it's a pretty obsolete viewpoint today.

###############################################################################

## End of file.
