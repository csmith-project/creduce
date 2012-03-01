#!/usr/bin/perl -w

package creduce_indent;

use strict;
use creduce_utils;

my $INDENT_OPTS = "-nbad -nbap -nbbb -cs -pcs -prs -saf -sai -saw -sob -ss ";

my $which = 0;

sub init () {
    $which = 0;
}

sub transform ($$) {
    (my $cfile, my $index) = @_;
    return $STOP unless ($index == 0 && $which == 0);
    system "indent $INDENT_OPTS $cfile";
    $which++;
    return $SUCCESS;
}

1;
