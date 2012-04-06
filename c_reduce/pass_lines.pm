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

my $index;
my $chunk_size;

sub reset ($$) {
    (my $cfile, my $arg) = @_;
    my $tmpfile = POSIX::tmpnam();
    system "topformflat $arg < $cfile > $tmpfile";
    system "mv $tmpfile $cfile";	
    $index = count_lines($cfile);
    $chunk_size = $index;
}

sub advance () {
    $index -= $chunk_size;
}

sub round ($) {
    (my $n) = @_;
    return int ($n+0.5);
}

sub transform ($$) {
    (my $cfile, my $arg) = @_;

    my $n=0;
    my $did_something=0;
    my $tmpfile = POSIX::tmpnam();
    open INF, "<$cfile" or die;
    open OUTF, ">$tmpfile" or die;
    while (my $line = <INF>) {
	if ($n < ($index - $chunk_size) ||
	    $n >= $index) {
	    print OUTF $line;
	} else {
	    $did_something++;
	}
	$n++;
    }
    close INF;
    close OUTF;
    
    # print "chunk= $chunk_size, index= $index, did_something= $did_something\n";

    if ($did_something) {
	system "mv $tmpfile $cfile";
    } else {
	system "rm $tmpfile";
	return $STOP if ($chunk_size == 1);
	$chunk_size = round ($chunk_size / 2.0);
	$index = count_lines($cfile);
    }

    return $OK;
}

1;
