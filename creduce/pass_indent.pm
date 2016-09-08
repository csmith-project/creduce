## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2013, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_indent;

use strict;
use warnings;

use creduce_config qw(CLANG_FORMAT);
use creduce_utils;

my $clang_format;

sub check_prereqs () {
    $clang_format =
	find_external_program(creduce_config::CLANG_FORMAT, "clang-format");
    return defined ($clang_format);
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

sub invoke_clang_format ($$) {
    (my $cfile, my $arg) = @_;
    if ($^O eq "MSWin32") {
	system qq{"$clang_format" -i $arg $cfile > NUL 2>&1};
    } else {
	system qq{"$clang_format" -i $arg $cfile >/dev/null 2>&1};
    }
}

my $spaces = "-style='{SpacesInAngles: true}'";

sub transform ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my $index = ${$state};
    my $old = read_file($cfile);
  AGAIN:
    return ($STOP, \$index) unless ($index == 0);
    if ($arg eq "regular") {
	invoke_clang_format($cfile, $spaces);
    } elsif ($arg eq "final") {
        invoke_clang_format($cfile, "");
    } else {
        die;
    }
    my $new = read_file($cfile);
    if ($old eq $new) {
	$index++;
	goto AGAIN;
    }

    return ($OK, \$index);
}

1;
