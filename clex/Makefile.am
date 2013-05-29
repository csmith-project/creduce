## -*- mode: Makefile-Automake -*-
##
## Copyright (c) 2013 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

libexec_PROGRAMS = clex

GIT_HASH := $(shell "$(top_srcdir)/git-hash.sh" "$(top_srcdir)" || echo error)
GIT_FLAG = -DGIT_VERSION=\"$(GIT_HASH)\"

clex_CPPFLAGS = \
	$(GIT_FLAG)

clex_SOURCES = \
	clex.l

###############################################################################

## End of file.
