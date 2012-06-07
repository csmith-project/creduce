## -*- mode: Perl -*-
##
## Copyright (c) 2012 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_indent;

use strict;
use warnings;

use creduce_utils;

my $INDENT_OPTS = "-nbad -nbap -nbbb -cs -pcs -prs -saf -sai -saw -sob -ss ";

sub check_prereqs () {
    my $path1 = File::Which::which ("indent");
    my $path2 = File::Which::which ("astyle");
    return defined ($path1) && defined ($path2);
}

sub new ($$) {
    my $index = 0;
    return \$index;
}

sub advance ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my $index = ${$state};
    $index++;
    return \$index;
}

sub transform ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my $index = ${$state};
    return ($STOP, \$index) unless ($index == 0);
    if (0) {
    } elsif ($arg eq "regular") {
	system "indent $INDENT_OPTS $cfile";
    } elsif ($arg eq "final") {
	system "indent $cfile";
	system "astyle $cfile >/dev/null 2>&1";
    } else {
	die;
    }
    $index++;
    return ($OK, \$index);
}

1;
