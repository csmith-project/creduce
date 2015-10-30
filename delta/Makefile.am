## -*- mode: Makefile-Automake -*-
##
## Copyright (c) 2015 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

libexec_PROGRAMS = topformflat

GIT_HASH := $(shell "$(top_srcdir)/git-hash.sh" "$(top_srcdir)" || echo error)
GIT_FLAG = -DGIT_VERSION=\"$(GIT_HASH)\"

topformflat_CPPFLAGS = \
	$(GIT_FLAG)

topformflat_SOURCES = \
	topformflat.l

# SHORT STORY: Always rebuild `topformflat.c' from `topformflat.lex' if Make
# thinks that `topformflat.c' is out of date.  This is achieved by a sinful
# Automake hack.
#
am__skiplex :=
#
# LONG STORY: See the long story in "clex/Makefile.am".

###############################################################################

## End of file.
