## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2013 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_clang_binsrch;

use strict;
use warnings;

use POSIX;

use File::Copy;
use File::Spec;

use creduce_config qw(bindir libexecdir);
use creduce_regexes;
use creduce_utils;

# `$clang_delta' is initialized by `check_prereqs()'.
my $clang_delta = "clang_delta";

my $ORIG_DIR;

sub count_lines ($) {
    (my $cfile) = @_;
    open INF, "$clang_delta  --query-instances=replace-function-def-with-decl $cfile |" or die;
    my $line = <INF>;
    die unless $line =~ /Available transformation instances: [(0-9)+]$/;
    my $n = $1;
    close INF;
    return $n;
}

sub check_prereqs () {
    $ORIG_DIR = getcwd();
    my $path;
    if ($FindBin::Bin eq bindir) {
	# This script is in the installation directory.
	# Use the installed `clang_delta'.
	$path = libexecdir . "/clang_delta";
    } else {
	# Assume that this script is in the C-Reduce build tree.
	# Use the `clang_delta' that is also in the build tree.
	$path = "$FindBin::Bin/../clang_delta/clang_delta";
    }
    if ((-e $path) && (-x $path)) {
	$clang_delta = $path;
	return 1;
    }
    return 0;
}

sub new ($$) {
    (my $cfile, my $arg) = @_;
    my %sh;
    return \%sh;
}

sub advance ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my %sh = %{$state};
    return \%sh if defined($sh{"start"});
    $sh{"index"} += $sh{"chunk"};
    return \%sh;
}


sub transform ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my %sh = %{$state};

    if (defined $sh{"flatten"}) {
	delete $sh{"flatten"};
	$sh{"start"} = 1;
	my $tmpfile = POSIX::tmpnam();
	system "$topformflat $arg < $cfile > $tmpfile";
	system "mv $tmpfile $cfile";	
	print "ran $topformflat $arg < $cfile > $tmpfile\n" if $VERBOSE;
	return ($OK, \%sh);
    }

    if (defined($sh{"start"})) {
	delete $sh{"start"};
	my $chunk = count_lines($cfile);
	$sh{"chunk"} = $chunk;
	print "initial granularity = $chunk\n" if $VERBOSE;
	$sh{"index"} = 0;
    }

  AGAIN:

    my $n=0;
    my $did_something=0;
    my $tmpfile = POSIX::tmpnam();
    open INF, "<$cfile" or die;
    open OUTF, ">$tmpfile" or die;
    while (my $line = <INF>) {
	if ($n >= ($sh{"index"} + $sh{"chunk"}) ||
	    $n < $sh{"index"}
	    ) {
	    print OUTF $line;
	} else {
	    $did_something++;
	}
	$n++;
    }
    close INF;
    close OUTF;

    if ($did_something) {
	system "mv $tmpfile $cfile";
    } else {
	system "rm $tmpfile";
	return ($STOP, \%sh) if ($sh{"chunk"} == 1);
	my $newchunk = round ($sh{"chunk"} / 2.0);
	$sh{"chunk"} = $newchunk;
	print "granularity = $newchunk\n" if $VERBOSE;
	$sh{"index"} = 0;
	goto AGAIN;
    }

    # print "chunk= ".$sh{"chunk"}.", index= ".$sh{"index"}.", did_something= ".$did_something."\n";

    return ($OK, \%sh);
}

sub transform ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my $index = ${$state};
    my $tmpfile = POSIX::tmpnam();
    my $cmd = "$clang_delta --transformation=$which --counter=$index $cfile";
    print "$cmd\n" if $VERBOSE;
    my $res = runit ("$cmd > $tmpfile");
    if ($res==0) {
	system "mv $tmpfile $cfile";
	return ($OK, \$index);
    } else {
	if ($res == -1) {
	} else {
	    my $crashfile = $tmpfile;
	    $crashfile =~ s/\//_/g;
	    my ($suffix) = $cfile =~ /(\.[^.]+)$/;
	    $crashfile = "clang_delta_crash" . $crashfile . $suffix;
	    my $crashfile_path = File::Spec->join($ORIG_DIR, $crashfile);
	    File::Copy::copy($cfile, $crashfile_path);
	    open TMPF, ">>$crashfile_path";
	    print TMPF "\n\n";
	    print TMPF "\/\/ this should reproduce the crash:\n";
	    print TMPF "\/\/ $clang_delta --transformation=$which --counter=$index $crashfile_path\n";
	    close TMPF;
	    print "\n\n=======================================\n\n";
	    print "OOPS: clang_delta crashed; please consider mailing\n";
	    print "${crashfile}\n";
	    print "to creduce-bugs\@flux.utah.edu and we will try to fix the bug\n";
	    print "please also let us know what version of C-Reduce you are using\n";
	    print "\n=======================================\n\n";
	}
	system "rm $tmpfile";
	return ($STOP, \$index);
    }    
}

1;
