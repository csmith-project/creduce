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

my $index;

sub reset () {
    $index = 0;
}

sub advance () {
    $index++;
}

sub transform ($$) {
    (my $cfile, my $which) = @_;

    my $prog = read_file ($cfile);
    my $prog2 = $prog;

    $replace_cont = -1;
    if ($which eq "b") {
	$prog2 =~ s/(?<all>(?<del1>$borderorspc)(?<a>$varnumexp)\s*\?\s*(?<b>$varnumexp)\s*:\s*(?<c>$varnumexp)(?<del2>$borderorspc))/replace_aux($index,$+{all},$+{del1}.$+{b}.$+{del2})/seg;
    } elsif ($which eq "c") {
	$prog2 =~ s/(?<all>(?<del1>$borderorspc)(?<a>$varnumexp)\s*\?\s*(?<b>$varnumexp)\s*:\s*(?<c>$varnumexp)(?<del2>$borderorspc))/replace_aux($index,$+{all},$+{del1}.$+{c}.$+{del2})/seg;
    } else {
	die "ternary pass didn't expect arg == $which";
    }

    if ($prog ne $prog2) {
	write_file ($cfile, $prog2);
	return $SUCCESS;
    } else {
	return $STOP;
    }
}

1;
