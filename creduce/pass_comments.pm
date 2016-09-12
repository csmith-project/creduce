## -*- mode: Perl -*-
##
## Copyright (c) 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_comments;

use strict;
use warnings;

use creduce_utils;
use Regexp::Common qw /comment/;

sub check_prereqs () {
    return 1;
}

sub new ($$) {
    my $index = -2;
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

  AGAIN:
    if ($index == -2) {
	# remove all C-style comments
	$prog2 =~ s#/\*[^*]*\*+([^/*][^*]*\*+)*/|("(\\.|[^"\\])*"|'(\\.|[^'\\])*'|.[^/"'\\]*)#defined $2 ? $2 : ""#gse;
    } elsif ($index == -1) {
	# remove all C++-style comments
	$prog2 =~ s/\/\/(.*?)$//gm;
    } else {
	# TODO remove only the nth comment
    }
    if (($prog eq $prog2) && ($index == -2)) {
	$index = -1;
	goto AGAIN;
    }
    
    if ($prog ne $prog2) {
	write_file ($cfile, $prog2);
	return ($OK, \$index);
    } else {
	return ($STOP, \$index);
    }
}

1;
