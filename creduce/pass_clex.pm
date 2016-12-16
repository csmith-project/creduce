## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2013, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_clex;

use strict;
use warnings;

use POSIX;

use Cwd 'abs_path';
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
    my $abs_bindir = abs_path(bindir);
    if ((defined $abs_bindir) && ($FindBin::RealBin eq $abs_bindir)) {
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
    # Check Windows
    $path=$path . ".exe";
    if (($^O eq "MSWin32") && (-e $path) && (-x $path)) {
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
    my $tmpfile = File::Temp::tmpnam();
    my $cmd = qq{"$clex" $which $index $cfile};
    print "$cmd\n" if $DEBUG;
    system ("$cmd > $tmpfile");
    my $res = $? >> 8;
    if ($res == 51) {
	File::Copy::move($tmpfile, $cfile);
	return ($OK, \$index);
    } elsif ($res == 71) {
	unlink $tmpfile;
	return ($STOP, \$index);
    } else {
	return ($ERROR, "crashed: $cmd");
    }    
}

1;
