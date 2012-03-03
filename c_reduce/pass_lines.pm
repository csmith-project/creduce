#!/usr/bin/perl -w

package pass_lines;

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
    undefine ($started);
}

	    if ($delta_method =~ /^lines/ && $chunk_size > 1) {
		$chunk_size = round ($chunk_size / 2.0);
		printf "new chunk size = $chunk_size\n" unless $QUIET;
		$delta_pos = 0;
		goto again;
	    }


my $chunk_size;

sub lines ($) {
    (my $chunk_size) = @_;

    my $chunk_start = $delta_pos * $chunk_size;

    open INF, "<$cfile" or die;
    open OUTF, ">tmpfile" or die;

    my $n=0;
    my $did_something=0;

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
	system "mv tmpfile $cfile";
	$changed_on_disk = 1;
	$delta_worked |= delta_test ();
    } else {
	$exit_delta_pass = 1;
    }
}

sub transform ($$$) {
    (my $cfile, my $index, my $arg) = @_;

    if (!(defined ($started))) {
	$started = 1;
	my $tmpfile = POSIX::tmpnam();
	system "topformflat $arg < $cfile > $tmpfile";
	system "mv tmpfile $cfile";	
	$chunk_size = count_lines($cfile);
	return $SUCCESS;
    }

    return $STOP;

    return $SUCCESS;
}

1;
