#!/usr/bin/perl -w

package pass_ints;

use strict;

use creduce_regexes;
use creduce_utils;

sub check_prereqs () {
    return 1;
}

my $index;
my $fail;

sub reset () {
    $index = 0;
    $fail = 0;
}

sub advance () {
    $index++;
}

sub transform ($$) {
    (my $cfile, my $which) = @_;

    my $prog = read_file ($cfile);
    my $prog2 = $prog;

    $replace_cont = -1;
    $matched = 0;
    
    # need some abstraction over the number patterns...

    if (0) {
    } elsif ($which eq "a") {
	$prog2 =~ s/(?<all>(?<pref>$borderorspc(\\-|\\+)?(0|(0[xX]))?)(?<del>[0-9a-fA-F])(?<numpart>[0-9a-fA-F]+)(?<suf>[ULul]*$borderorspc))/replace_aux($index,$+{all},$+{pref}.$+{numpart}.$+{suf})/egs;
    } elsif ($which eq "b") {
	$prog2 =~ s/(?<all>(?<pref1>$borderorspc)(?<pref2>(\\-|\\+)?(0|(0[xX]))?)(?<numpart>[0-9a-fA-F]+)(?<suf>[ULul]*$borderorspc))/replace_aux($index,$+{all},$+{pref1}.$+{numpart}.$+{suf})/egs;
    } elsif ($which eq "c") {
	$prog2 =~ s/(?<all>(?<pref>$borderorspc(\\-|\\+)?(0|(0[xX]))?)(?<numpart>[0-9a-fA-F]+)(?<suf1>[ULul]*)(?<suf2>$borderorspc))/replace_aux($index,$+{all},$+{pref}.$+{numpart}.$+{suf2})/egs;
    } elsif ($which eq "d") {
	# hex to decimal
	$prog2 =~ s/(?<all>(?<pref>$borderorspc)(?<numpart>0[Xx][0-9a-fA-F]+)(?<ll>[ULul]*)(?<suf>$borderorspc))/replace_aux($index,$+{all},$+{pref}.hex($+{numpart}).$+{ll}.$+{suf})/egs;
    } elsif ($which eq "e") {
	# remove the UuLl suffixes
	$prog2 =~ s/(?<all>((?<pref>$borderorspc)(?<numpart>(\\-|\\+)?(0[Xx])?[0-9a-fA-F]+)[ULul]+(?<suf>$borderorspc)))/replace_aux($index,$+{all},$+{pref}.$+{numpart}.$+{suf})/egs;
    } else {
	die;
    }
    
    if ($prog ne $prog2) {
	write_file ($cfile, $prog2);
	return $SUCCESS;
    } else {
	return $STOP;
    }
}

1;
