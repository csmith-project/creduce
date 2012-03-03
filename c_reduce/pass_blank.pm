#!/usr/bin/perl -w

package pass_blank;

use strict;
use creduce_utils;

sub check_prereqs () {
    return 1;
}

sub init () {
}

sub transform ($$) {
    (my $cfile, my $index) = @_;

    my $prog = read_file ($cfile);
    my $prog2 = $prog;

    $replace_cont = -1;
    $prog2 =~ s/((\s\t){2,})/replace_aux($index,$1," ")/eg;

    if ($prog ne $prog2) {
	write_file ($cfile, $prog2);
	return $SUCCESS;
    } else {
	return $STOP;
    }
}

1;
