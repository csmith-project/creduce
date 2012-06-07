## -*- mode: Perl -*-
##
## Copyright (c) 2012 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_lines;

use strict;
use warnings;

use POSIX;
use File::Which;
use creduce_utils;

my $BACKWARD = 0;

sub count_lines ($) {
    (my $cfile) = @_;
    open INF, "<$cfile" or die;
    my $n=0;
    $n++ while (<INF>);
    close INF;
    return $n;
}

sub check_prereqs () {
    my $path = File::Which::which ("topformflat");
    return defined ($path);
}

sub new ($$) {
    (my $cfile, my $arg) = @_;
    my $tmpfile = POSIX::tmpnam();
    system "topformflat $arg < $cfile > $tmpfile";
    system "mv $tmpfile $cfile";	
    my %sh;
    if ($BACKWARD) {
	$sh{"chunk"} = count_lines($cfile);
	$sh{"index"} = $sh{"chunk"};
    } else {
	$sh{"chunk"} = count_lines($cfile);
	$sh{"index"} = 0;
    }
    return \%sh;
}

sub advance ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my %sh = %{$state};
    if ($BACKWARD) {
	$sh{"index"} -= $sh{"chunk"};
    } else {
	$sh{"index"} += $sh{"chunk"};
    }
    return \%sh;
}

sub round ($) {
    (my $n) = @_;
    return int ($n+0.5);
}

sub transform ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my %sh = %{$state};

    my $n=0;
    my $did_something=0;
    my $tmpfile = POSIX::tmpnam();
    open INF, "<$cfile" or die;
    open OUTF, ">$tmpfile" or die;
    while (my $line = <INF>) {
	if (($BACKWARD && (
		 $n < ($sh{"index"} - $sh{"chunk"}) ||
		 $n >= $sh{"index"}
	     )) ||
	    (!$BACKWARD && (
		 $n >= ($sh{"index"} + $sh{"chunk"}) ||
		 $n < $sh{"index"}
	     ))) {
	    print OUTF $line;
	} else {
	    $did_something++;
	}
	$n++;
    }
    close INF;
    close OUTF;
    
    # print "chunk= ".$sh{"chunk"}.", index= ".$sh{"index"}.", did_something= ".$did_something."\n";

    if ($did_something) {
	system "mv $tmpfile $cfile";
    } else {
	system "rm $tmpfile";
	return ($STOP, \%sh) if ($sh{"chunk"} == 1);
	$sh{"chunk"} = round ($sh{"chunk"} / 2.0);
	if ($BACKWARD) {
	    $sh{"index"} = count_lines($cfile);
	} else {
	    $sh{"index"} = 0;
	}
    }

    return ($OK, \%sh);
}

1;
