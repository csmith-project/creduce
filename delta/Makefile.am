## -*- mode: Makefile-Automake -*-
##
## Copyright (c) 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

libexec_PROGRAMS = topformflat

topformflat_CPPFLAGS =

topformflat_SOURCES = \
	topformflat.l

# SHORT STORY: Always rebuild `topformflat.c' from `topformflat.l' if Make
# thinks that `topformflat.c' is out of date.  This is achieved by a sinful
# Automake hack.
#
am__skiplex :=
#
# LONG STORY: See the long story in "clex/Makefile.am".

# C-Reduce doesn't use any of this stuff, but it's part of the third-party
# source, so we distribute it.
#
EXTRA_DIST = \
	Build.mk \
	CMakeLists.txt \
	License.txt \
	Readme \
	Test.mk \
	delta \
	multidelta \
	test0_delta \
	test1_multidelta \
	test2_multidelta \
	www

###############################################################################

## End of file.
