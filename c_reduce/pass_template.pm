#!/usr/bin/perl -w

package pass_template;

use strict;

use creduce_regexes;
use creduce_utils;

sub check_prereqs () {
    return 1;
}

my $index;

sub init () {
    $index = 0;
}

sub advance () {
    $index++;
}

sub transform ($$) {
    (my $cfile, my $which) = @_;

    my $prog = read_file ($cfile);
    my $prog2 = $prog;

    # do stuff

    if ($prog ne $prog2) {
	write_file ($cfile, $prog2);
	return $SUCCESS;
    } else {
	return $STOP;
    }
}

1;
