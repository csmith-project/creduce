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

sub new ($$) {
    my $index = 1;
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
    my $tmpfile = POSIX::tmpnam();
    my $cmd = "clang_delta --transformation=$which --counter=$index $cfile > $tmpfile";
    my $res = runit ($cmd);
    if ($res==0) {
	system "mv $tmpfile $cfile";
	return ($OK, \$index);
    } else {
	system "rm $tmpfile";
	return ($STOP, \$index);
    }    
}

1;
