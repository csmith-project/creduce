## -*- mode: Perl -*-
##
## Copyright (c) 2012 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_clang;

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
    my $cmd = "$clang_delta --transformation=$which --counter=$index $cfile";
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
	    print TMPF "\n\n\/\/ $cmd\n";
	    close TMPF;
	    print "\n\n=======================================\n\n";
	    print "OOPS: clang_delta crashed; please consider\n";
	    print "mailing ${crashfile} to creduce-bugs\@flux.utah.edu\n";
	    print "and we will try to fix the bug\n";
	    print "\n=======================================\n\n";
	}
	system "rm $tmpfile";
	return ($STOP, \$index);
    }    
}

1;
