## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2013, 2014, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_clang;

use strict;
use warnings;

use POSIX;

use Cwd 'abs_path';
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
    my $abs_bindir = abs_path(bindir);
    if ((defined $abs_bindir) && ($FindBin::RealBin eq $abs_bindir)) {
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
    # Check Windows
    $path=$path . ".exe";
    if (($^O eq "MSWin32") && (-e $path) && (-x $path)) {
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
    my $tmpfile = File::Temp::tmpnam();
    my $cmd = qq{"$clang_delta" --transformation=$which --counter=$index $cfile};
    print "$cmd\n" if $DEBUG;
    my $res = run_clang_delta ("$cmd > $tmpfile");
    if ($res==0) {
	File::Copy::move($tmpfile, $cfile);
	return ($OK, \$index);
    } else {
        unlink $tmpfile;
	if (($res != -1) && ($res != -2)) {
	    return ($ERROR, "crashed: $cmd");
        } else {
	    return ($STOP, \$index);
	}
    }
}

1;
