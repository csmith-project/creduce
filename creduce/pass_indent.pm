## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2013 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_indent;

use strict;
use warnings;

use creduce_config qw(ASTYLE CLANG_FORMAT INDENT);
use creduce_utils;

my $astyle;
my $clang_format;
my $indent;

my $indent_opts = "-nbad -nbap -nbbb -cs -pcs -prs -saf -sai -saw -sob -ss ";

sub check_prereqs () {
    $astyle =
	find_external_program(creduce_config::ASTYLE, "astyle");
    $clang_format =
	find_external_program(creduce_config::CLANG_FORMAT, "clang-format");
    $indent =
	find_external_program(creduce_config::INDENT, "indent");

    return defined ($astyle) && defined ($clang_format) && defined ($indent);
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
    if (0) {
    } elsif ($arg eq "regular") {
	return ($STOP, \$index) unless ($index == 0);
	system "$indent $indent_opts $cfile >/dev/null 2>&1";
    } elsif ($arg eq "final") {
	if ($index == 0) {
	    system "$indent $cfile >/dev/null 2>&1";
	} elsif ($index == 1) {
	    system "$astyle $cfile >/dev/null 2>&1";
	} elsif ($index == 2) {
	    system "$clang_format -i $cfile >/dev/null 2>&1";
	} else {
	    return ($STOP, \$index);
	}
    }
    $index++;
    return ($OK, \$index);
}

1;
