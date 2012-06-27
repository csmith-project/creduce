## -*- mode: Perl -*-
##
## Copyright (c) 2012 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_blank;

use strict;
use warnings;

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
    (my $cfile, my $arg, my $state) = @_;
    my $index = ${$state};

    return ($STOP, \$index) unless ($index < 2);

    if ($index == 0) {
	open INF, "<$cfile" or die;
	my $tmpfile = POSIX::tmpnam();
	open OUTF, ">$tmpfile" or die;
	while (my $line = <INF>) {
	    next if ($line =~ /^\s*$/);
	    print OUTF $line;
	}
	close INF;
	close OUTF;    
	system "mv $tmpfile $cfile";
    }

    if ($index == 1) {
	open INF, "<$cfile" or die;
	my $tmpfile = POSIX::tmpnam();
	open OUTF, ">$tmpfile" or die;
	while (my $line = <INF>) {
	    next if ($line =~ /^#/);
	    print OUTF $line;
	}
	close INF;
	close OUTF;    
	system "mv $tmpfile $cfile";
    }
    
    $index++;
    return ($OK, \$index);
}

1;
