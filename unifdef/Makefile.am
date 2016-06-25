## -*- mode: Makefile-Automake -*-
##
## Copyright (c) 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

libexec_PROGRAMS = unifdef

GIT_HASH := $(shell "$(top_srcdir)/git-hash.sh" "$(top_srcdir)" || echo error)
GIT_FLAG = -DGIT_VERSION=\"$(GIT_HASH)\"

unifdef_CPPFLAGS = \
	$(GIT_FLAG)

unifdef_SOURCES = \
	unifdef.c \
	unifdef.h \
	version.h

EXTRA_DIST = \
	CMakeLists.txt

###############################################################################

## End of file.
