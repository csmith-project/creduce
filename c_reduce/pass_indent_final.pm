#!/usr/bin/perl -w

package pass_indent_final;

use strict;
use creduce_utils;

my $which = 0;

sub check_prereqs () {
    my $path1 = File::Which::which ("indent");
    my $path2 = File::Which::which ("astyle");
    return defined ($path1) && defined ($path2);
}

sub init () {
    $which = 0;
}

sub transform ($$) {
    (my $cfile, my $index) = @_;
    return $STOP unless ($index == 0 && $which == 0);
    system "indent -nbad -nbap -nbbb $cfile";
    system "astyle -A2 -xd -s2 $cfile";
    $which++;
    return $SUCCESS;
}

1;
