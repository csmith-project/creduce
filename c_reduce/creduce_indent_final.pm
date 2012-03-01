#!/usr/bin/perl -w

package creduce_indent_final;

use strict;
use creduce_utils;

my $which = 0;

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
