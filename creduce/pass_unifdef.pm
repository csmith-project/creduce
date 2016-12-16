## -*- mode: Perl -*-
##
## Copyright (c) 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_unifdef;

use strict;
use warnings;

use POSIX;
use File::Compare;
use Cwd 'abs_path';

use creduce_config qw(bindir libexecdir);
use creduce_utils;

my $unifdef;
my $options = "-B -x 2";

sub check_prereqs () {
    my $path;
    my $abs_bindir = abs_path(bindir);
    if ((defined $abs_bindir) && ($FindBin::RealBin eq $abs_bindir)) {
	# This script is in the installation directory.
	# Use the installed `unifdef'.
	$path = libexecdir . "/unifdef";
    } else {
	# Assume that this script is in the C-Reduce build tree.
	# Use the `unifdef' that is also in the build tree.
	$path = "$FindBin::Bin/../unifdef/unifdef";
    }
    if ((-e $path) && (-x $path)) {
	$unifdef = $path;
	return 1;
    }
    # Check Windows
    $path = $path . ".exe";
    if (($^O eq "MSWin32") && (-e $path) && (-x $path)) {
	$unifdef = $path;
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
    my %defs;
    open INF, "$unifdef -s $cfile 2>/dev/null |" or die;
    while (my $line = <INF>) {
	chomp $line;
	$defs{$line} = 1;
    }
    close INF;
    my @deflist = sort keys %defs;
    my $tmpfile = File::Temp::tmpnam();
  AGAIN:
    print "index = $index\n" if $DEBUG;
    my $DU = (($index % 2) == 0) ? "-D" : "-U";
    # my $DU = (($index % 2) == 0) ? "-U" : "-D";
    my $n_index = int($index / 2);
    return ($STOP, \$index) unless ($n_index < scalar(@deflist));
    my $def = $deflist[$n_index];
    my $cmd = "$unifdef $options $DU$def -o $tmpfile $cfile >/dev/null 2>&1";
    print "$cmd\n" if $DEBUG;
    my $res = runit ($cmd);
    if (compare($cfile, $tmpfile) == 0) {
	$index++;
	print "AGAIN!\n" if $DEBUG;
	goto AGAIN;
    }
    File::Copy::move($tmpfile, $cfile);
    return ($OK, \$index);
}

1;
