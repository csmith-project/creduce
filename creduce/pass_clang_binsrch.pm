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

sub count_instances ($$) {
    (my $cfile, my $which) = @_;
    open INF, "$clang_delta --query-instances=$which $cfile |" or die;
    my $line = <INF>;
    die unless $line =~ /Available transformation instances: ([0-9]+)$/;
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
    (my $cfile, my $which) = @_;
    my %sh;
    $sh{"start"} = 1;
    return \%sh;
}

sub advance ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my %sh = %{$state};
    return \%sh if defined($sh{"start"});
    $sh{"index"} += $sh{"chunk"};
    if ($VERBOSE) {
	my $index = $sh{"index"};
	my $chunk = $sh{"chunk"};
	print "ADVANCE: index = $index, chunk = $chunk\n";
    }
    return \%sh;
}

sub round ($) {
    (my $n) = @_;
    return int ($n+0.5);
}

sub transform ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my %sh = %{$state};

    if (defined($sh{"start"})) {
	delete $sh{"start"};
	my $instances = count_instances($cfile,$which);
	$sh{"chunk"} = $instances;
	$sh{"instances"} = $instances;
	print "initial granularity = $instances\n" if $VERBOSE;
	$sh{"index"} = 1;
    }

  AGAIN:

    my $n=0;
    my $index = $sh{"index"};
    my $chunk = $sh{"chunk"};
    my $instances = $sh{"instances"};
    my $tmpfile = POSIX::tmpnam();

    print "TRANSFORM: index = $index, chunk = $chunk, instances = $instances\n" if $VERBOSE;

    if ($index <= $instances) {
	my $end = $index + $chunk;
	if ($end > $instances) {
	    $end = $instances;
	}
	
	my $dec = $end - $index + 1;

	my $cmd = "$clang_delta --transformation=$which --counter=$index --to-counter=$end $cfile";
	print "$cmd\n" if $VERBOSE;
	my $res = runit ("$cmd > $tmpfile");

	if ($res==0) {
	    system "mv $tmpfile $cfile";
	    return ($OK, \%sh);
	} else {
	    if ($res == -1) {
		# nothing?
	    } elsif ($res == -2) {
		print "out of instances!\n" if $VERBOSE;
		goto rechunk;
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
	    return ($STOP, \%sh);
	}    	
	system "mv $tmpfile $cfile";
    } else {
      rechunk:
	return ($STOP, \%sh) if ($sh{"chunk"} < 10);
	my $newchunk = round ($sh{"chunk"} / 2.0);
	$sh{"chunk"} = $newchunk;
	print "granularity = $newchunk\n" if $VERBOSE;
	$sh{"index"} = 1;
	goto AGAIN;
    }
    return ($OK, \%sh);
}

1;
