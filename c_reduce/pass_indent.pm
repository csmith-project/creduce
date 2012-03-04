#!/usr/bin/perl -w

package pass_indent;

use strict;
use creduce_utils;

my $INDENT_OPTS = "-nbad -nbap -nbbb -cs -pcs -prs -saf -sai -saw -sob -ss ";

my $which = 0;
my $index;

sub advance () {
    $index++;
}

sub check_prereqs () {
    my $path1 = File::Which::which ("indent");
    my $path2 = File::Which::which ("astyle");
    return defined ($path1) && defined ($path2);
}

sub init () {
    $which = 0;
    $index = 0;
}

sub transform ($$) {
    (my $cfile, my $arg) = @_;
    return $STOP unless ($index == 0 && $which == 0);
    if (0) {
    } elsif ($arg eq "regular") {
	system "indent $INDENT_OPTS $cfile";
    } elsif ($arg eq "final") {
	system "indent -nbad -nbap -nbbb $cfile";
	system "astyle -A2 -xd -s2 $cfile >/dev/null 2>&1";
    } else {
	die;
    }
    $which++;
    return $SUCCESS;
}

1;
