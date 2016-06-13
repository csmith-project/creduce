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

use creduce_config qw(ASTYLE CLANG_FORMAT INDENT);
use creduce_utils;

my $astyle;
my $clang_format;
my $indent;

my $astyle_opts = "--options=none";
my $indent_opts = "-npro -nbad -nbap -nbbb -cs -pcs -prs -saf -sai -saw -sob -ss ";

sub check_prereqs () {
    $astyle =
	find_external_program(creduce_config::ASTYLE, "astyle");
    $clang_format =
	find_external_program(creduce_config::CLANG_FORMAT, "clang-format");
    $indent =
	find_external_program(creduce_config::INDENT, "indent");

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

sub advance_on_success ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my $index = ${$state};
    $index++;
    return \$index;
}

sub invoke_indent ($) {
    (my $cfile) = @_;
    if ($^O eq "MSWin32") {
	system qq{"$indent" $indent_opts $cfile > NUL 2>&1};
    } else {
	system qq{"$indent" $indent_opts $cfile >/dev/null 2>&1};
    }
}

sub invoke_astyle ($) {
    (my $cfile) = @_;
    if ($^O eq "MSWin32") {
	system qq{"$astyle" $astyle_opts $cfile > NUL 2>&1};
    } else {
	system qq{"$astyle" $astyle_opts $cfile >/dev/null 2>&1};
    }
}

sub invoke_clang_format ($) {
    (my $cfile) = @_;
    if ($^O eq "MSWin32") {
	system qq{"$clang_format" -i $cfile > NUL 2>&1};
    } else {
	system qq{"$clang_format" -i $cfile >/dev/null 2>&1};
    }
}

sub transform ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my $index = ${$state};
    my $old = read_file($cfile);
  AGAIN:
    if ($arg eq "regular") {
	return ($STOP, \$index) unless ($index == 0);
	invoke_clang_format($cfile);
    } elsif ($arg eq "final") {
	if ($index == 0) {
	    invoke_indent($cfile) if defined ($indent);
	} elsif ($index == 1) {
	    invoke_astyle($cfile) if defined ($astyle);
	} elsif ($index == 2) {
	    invoke_clang_format($cfile);
	} else {
	    return ($STOP, \$index);
	}
    }
    my $new = read_file($cfile);
    if ($old eq $new) {
	$index++;
	goto AGAIN;
    }

    return ($OK, \$index);
}

1;
