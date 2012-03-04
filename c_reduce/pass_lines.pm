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

sub init () {
    undef ($started);
}

my $chunk_size;
my $diff;

sub round ($) {
    (my $n) = @_;
    return int ($n+0.5);
}

sub transform ($$$) {
    (my $cfile, my $index, my $arg) = @_;

    if (!(defined ($started))) {
	$started = 1;
	my $tmpfile = POSIX::tmpnam();
	system "topformflat $arg < $cfile > $tmpfile";
	system "mv $tmpfile $cfile";	
	$chunk_size = count_lines($cfile);
	$diff = 0;
	return $SUCCESS;
    }

    $index -= $diff;
    die if ($index < 0);
    
    my $chunk_start = $index * $chunk_size;
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
	return $STOP if ($chunk_size == 1);
	$chunk_size = round ($chunk_size / 2.0);
	$diff += $index;
	printf "new chunk size = $chunk_size\n";
    }

    return $SUCCESS;
}

1;
