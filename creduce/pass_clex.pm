## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2013 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_clex;

use strict;
use warnings;

use POSIX;

use File::Copy;
use File::Spec;

use creduce_config qw(bindir libexecdir);
use creduce_regexes;
use creduce_utils;

# `$clex' is initialized by `check_prereqs()'.
my $clex = "clex";

my $ORIG_DIR;

sub check_prereqs () {
    $ORIG_DIR = getcwd();
    my $path;
    if ($FindBin::RealBin eq bindir) {
	# This script is in the installation directory.
	# Use the installed `clex'.
	$path = libexecdir . "/clex";
    } else {
	# Assume that this script is in the C-Reduce build tree.
	# Use the `clex' that is also in the build tree.
	$path = "$FindBin::Bin/../clex/clex";
    }
    if ((-e $path) && (-x $path)) {
	$clex = $path;
	return 1;
    }
    return 0;
}

sub new ($$) {
    my $index = 0;
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
    my $cmd = "$clex $which $index $cfile";
    print "$cmd\n" if $VERBOSE;
    my $res = runit ("$cmd > $tmpfile");
    if ($res==0) {
	system "mv $tmpfile $cfile";
	return ($OK, \$index);
    } else {
	if ($res == -1) {
	} else {
	    # TODO -- log a crash like we do for clang-delta
	}
	system "rm $tmpfile";
	return ($STOP, \$index);
    }    
}

1;
