#!/usr/bin/perl -w

package pass_lines;

use POSIX;
use File::Which;
use strict;
use creduce_utils;

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
    $sh{"index"} = count_lines($cfile);
    $sh{"chunk"} = $sh{"index"};
    return \%sh;
}

sub advance ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my %sh = %{$state};
    $sh{"index"} -= $sh{"chunk"};
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
	if ($n < ($sh{"index"} - $sh{"chunk"}) ||
	    $n >= $sh{"index"}) {
	    print OUTF $line;
	} else {
	    $did_something++;
	}
	$n++;
    }
    close INF;
    close OUTF;
    
    # print "chunk= $sh{"chunk"}, index= $sh{"index"}, did_something= $did_something\n";

    if ($did_something) {
	system "mv $tmpfile $cfile";
    } else {
	system "rm $tmpfile";
	return ($STOP, \%sh) if ($sh{"chunk"} == 1);
	$sh{"chunk"} = round ($sh{"chunk"} / 2.0);
	$sh{"index"} = count_lines($cfile);
    }

    return ($OK, \%sh);
}

1;
