#!/usr/bin/perl -w
##
## Copyright (c) 2015 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

use strict;

my @deps = ("diff", "indent", "astyle", "topformflat", "clang_format");
my @progs = ("clang_delta", "clex", "strlex");

my $target = $ARGV[0];

###############################################################################

## End of file.
