#!/usr/bin/perl -w

package creduce_ternary;

use Regexp::Common;
use re 'eval';

use strict;

use creduce_regexes;
use creduce_utils;

sub init () {
}

sub transform ($$) {
    (my $cfile, my $index) = @_;

    my $string = read_file ($cfile);
    my $string2 = $string;

    my $which = $index % 2;
    $index /= 2;

    $replace_cont = -1;
    if ($which == 0) {
	$string2 =~ s/(?<all>(?<del1>$borderorspc)(?<a>$varnumexp)\s*\?\s*(?<b>$varnumexp)\s*:\s*(?<c>$varnumexp)(?<del2>$borderorspc))/replace_aux($index,$+{all},$+{del1}.$+{b}.$+{del2})/eg;
    } else {
	$string2 =~ s/(?<all>(?<del1>$borderorspc)(?<a>$varnumexp)\s*\?\s*(?<b>$varnumexp)\s*:\s*(?<c>$varnumexp)(?<del2>$borderorspc))/replace_aux($index,$+{all},$+{del1}.$+{c}.$+{del2})/eg;
    }

    if ($string ne $string2) {
	write_file ($cfile, $string2);
	return $SUCCESS;
    } else {
	return $STOP;
    }
}

1;
