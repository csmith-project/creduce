#!/usr/bin/perl -w

package pass_crc;

use strict;

use creduce_regexes;
use creduce_utils;

sub check_prereqs () {
    return 1;
}

my $index;

sub reset ($$) {
    $index = 0;
}

sub advance () {
    $index++;
}

sub junk ($) {
    (my $list) = @_;
    my @stuff = split /,/, $list;
    my $var = $stuff[0];
    return "printf (\"%d\\n\", (int)$var)";
}

sub transform ($$) {
    (my $cfile, my $which) = @_;

    my $prog = read_file ($cfile);
    my $prog2 = $prog;

    # this only makes sense for Csmith output...
    $replace_cont = -1;
    $prog2 =~ s/(?<all>transparent_crc\s*\((?<list>.*?)\))/replace_aux($index,$+{all},junk($+{list}))/egs;

    if ($prog ne $prog2) {
	write_file ($cfile, $prog2);
	return $SUCCESS;
    } else {
	return $STOP;
    }
}

1;
