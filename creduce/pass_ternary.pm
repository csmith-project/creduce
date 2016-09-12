## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_ternary;

use strict;
use warnings;

use Regexp::Common;
use re 'eval';

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
    (my $cfile, my $which, my $state) = @_;
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
    if ($which eq "b") {
	$prog2 =~ s/(?<all>(?<del1>$borderorspc)(?<a>$varnumexp)\s*\?\s*(?<b>$varnumexp)\s*:\s*(?<c>$varnumexp)(?<del2>$borderorspc))/replace_aux($index,$+{all},$+{del1}.$+{b}.$+{del2})/seg;
    } elsif ($which eq "c") {
	$prog2 =~ s/(?<all>(?<del1>$borderorspc)(?<a>$varnumexp)\s*\?\s*(?<b>$varnumexp)\s*:\s*(?<c>$varnumexp)(?<del2>$borderorspc))/replace_aux($index,$+{all},$+{del1}.$+{c}.$+{del2})/seg;
    } else {
	die "ternary pass didn't expect arg == $which";
    }

    if ($prog ne $prog2) {
	write_file ($cfile, $prog2);
	return ($OK, \$state);
    } else {
	return ($STOP, \$state);
    }
}

1;
