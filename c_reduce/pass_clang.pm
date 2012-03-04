#!/usr/bin/perl -w

package pass_clang;

use POSIX;
use File::Which;
use strict;

use creduce_regexes;
use creduce_utils;

sub check_prereqs () {
    my $path = File::Which::which ("clang_delta");
    return defined ($path);
}

my $index;

sub init () {
    $index = 1;
}

sub advance () {
    $index++;
}

sub transform ($$) {
    (my $cfile, my $which) = @_;

    my $tmpfile = POSIX::tmpnam();
    my $cmd = "clang_delta --transformation=$which --counter=$index $cfile > $tmpfile";
    my $res = runit ($cmd);
    if ($res==0) {
	system "mv $tmpfile $cfile";
	return $SUCCESS;
    } else {
	system "rm $tmpfile";
	return $STOP;
    }    
}

1;
