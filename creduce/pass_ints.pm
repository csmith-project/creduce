## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_ints;

use strict;
use warnings;
no warnings 'portable';

use creduce_regexes;
use creduce_utils;

sub check_prereqs () {
    return 1;
}

sub new ($$) {
    my $index = 0;
    return \$index;
}

sub advance ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my $index = ${$state};
    $index++;
    return \$index;
}

sub transform ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my $index = ${$state};

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
	return ($OK, \$index);
    } else {
	return ($STOP, \$index);
    }
}

1;
