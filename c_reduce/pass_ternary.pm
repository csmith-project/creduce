#!/usr/bin/perl -w

package pass_ternary;

use Regexp::Common;
use re 'eval';

use strict;

use creduce_regexes;
use creduce_utils;

sub check_prereqs () {
    return 1;
}

sub init () {
}

sub transform ($$$) {
    (my $cfile, my $index, my $which) = @_;

    my $string = read_file ($cfile);
    my $string2 = $string;

    $replace_cont = -1;
    if ($which eq "b") {
	$string2 =~ s/(?<all>(?<del1>$borderorspc)(?<a>$varnumexp)\s*\?\s*(?<b>$varnumexp)\s*:\s*(?<c>$varnumexp)(?<del2>$borderorspc))/replace_aux($index,$+{all},$+{del1}.$+{b}.$+{del2})/eg;
    } elsif ($which eq "c") {
	$string2 =~ s/(?<all>(?<del1>$borderorspc)(?<a>$varnumexp)\s*\?\s*(?<b>$varnumexp)\s*:\s*(?<c>$varnumexp)(?<del2>$borderorspc))/replace_aux($index,$+{all},$+{del1}.$+{c}.$+{del2})/eg;
    } else {
	die "ternary pass didn't expect arg == $which";
    }

    if ($string ne $string2) {
	write_file ($cfile, $string2);
	return $SUCCESS;
    } else {
	return $STOP;
    }
}

1;
