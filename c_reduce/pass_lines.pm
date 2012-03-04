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

my $started;
my $index;

sub init () {
    $index = 0;
    undef ($started);
}

sub advance () {
    $index++;
}

my $chunk_size;
my $diff;

sub round ($) {
    (my $n) = @_;
    return int ($n+0.5);
}

sub transform ($$) {
    (my $cfile, my $arg) = @_;

    if (!(defined ($started))) {
	$started = 1;
	my $tmpfile = POSIX::tmpnam();
	system "topformflat $arg < $cfile > $tmpfile";
	system "mv $tmpfile $cfile";	
	$chunk_size = count_lines($cfile);
	$diff = 0;
	return $SUCCESS;
    }

    my $pos = $index - $diff;
    die if ($pos < 0);
    
    my $chunk_start = $pos * $chunk_size;
    my $n=0;
    my $did_something=0;
    my $tmpfile = POSIX::tmpnam();
    open INF, "<$cfile" or die;
    open OUTF, ">$tmpfile" or die;
    while (my $line = <INF>) {
	if ($n < $chunk_start ||
	    $n >= ($chunk_start + $chunk_size)) {
	    print OUTF $line;
	} else {
	    chomp $line;
	    $did_something = 1;
	}
	$n++;
    }
    close INF;
    close OUTF;

    if ($did_something) {
	system "mv $tmpfile $cfile";
    } else {
	system "rm $tmpfile";
	return $STOP if ($chunk_size == 1);
	$chunk_size = round ($chunk_size / 2.0);
	$diff += $pos;
    }

    return $SUCCESS;
}

1;
