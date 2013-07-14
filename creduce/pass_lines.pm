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
    my %sh;
    $sh{"flatten"} = 1;
    return \%sh;
}

sub advance ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my %sh = %{$state};

    return \%sh if defined($sh{"start"});

    if ($BACKWARD) {
	$sh{"index"} -= $sh{"chunk"};
    } else {
	$sh{"index"} += $sh{"chunk"};
    }
    my $foo = $sh{"index"};
    # print "advanced index to $foo\n";
    return \%sh;
}

sub round ($) {
    (my $n) = @_;
    return int ($n+0.5);
}

sub transform ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my %sh = %{$state};

    if (defined $sh{"flatten"}) {
	delete $sh{"flatten"};
	$sh{"start"} = 1;
	my $tmpfile = POSIX::tmpnam();
	system "topformflat $arg < $cfile > $tmpfile";
	system "mv $tmpfile $cfile";	
	print "ran topformflat $arg < $cfile > $tmpfile\n" if $VERBOSE;
	return ($OK, \%sh);
    }

    if (defined($sh{"start"})) {
	delete $sh{"start"};
	my $chunk = count_lines($cfile);
	$sh{"chunk"} = $chunk;
	print "initial granularity = $chunk\n" if $VERBOSE;
	if ($BACKWARD) {
	    $sh{"index"} = $chunk;
	} else {
	    $sh{"index"} = 0;
	}
    }

  AGAIN:

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

    # OI, STUPID HACK
    if ($BACKWARD &&
	!$did_something &&
	$sh{"index"} >= 0) {
	my $newsh = advance ($cfile, 0, \%sh);
	%sh = %{$newsh};
	goto AGAIN;
    }
    
    if ($did_something) {
	system "mv $tmpfile $cfile";
    } else {
	system "rm $tmpfile";
	return ($STOP, \%sh) if ($sh{"chunk"} == 1);
	my $newchunk = round ($sh{"chunk"} / 2.0);
	$sh{"chunk"} = $newchunk;
	print "granularity = $newchunk\n" if $VERBOSE;
	if ($BACKWARD) {
	    $sh{"index"} = count_lines($cfile);
	} else {
	    $sh{"index"} = 0;
	}
	goto AGAIN;
    }

    # print "chunk= ".$sh{"chunk"}.", index= ".$sh{"index"}.", did_something= ".$did_something."\n";

    return ($OK, \%sh);
}

1;
