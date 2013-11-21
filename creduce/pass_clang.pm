## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2013 The University of Utah
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
    if ($FindBin::RealBin eq bindir) {
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
    print "$cmd\n" if $VERBOSE;
    my $res = run_clang_delta ("$cmd > $tmpfile");
    if ($res==0) {
	system "mv $tmpfile $cfile";
	return ($OK, \$index);
    } else {
	if (($res == -1) || ($res == -2)) {
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
