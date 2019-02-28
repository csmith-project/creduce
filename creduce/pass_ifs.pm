## -*- mode: Perl -*-
##
## Copyright (c) 2019 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_ifs;

use strict;
use warnings;

use POSIX;
use File::Copy;
use File::Compare;
use creduce_utils;
use Cwd 'abs_path';

use creduce_config qw(bindir libexecdir);

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

sub count_ifs($$) {
    my ($cfile, $which) = @_;
    open INF, "<$cfile" or die;
    my $n = 0;
    while (my $line = <INF>) {
        if ($line =~ m/^\s*#\s*if/) {
            $n++;
        }
    }
    close INF;
    return $n;
}

sub new ($$) {
    my ($cfile, $arg) = @_;
    my %sh;
    $sh{"start"} = 1;
    return \%sh;
}

sub advance ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my %sh = %{$state};
    die if (defined($sh{"start"}));
    my $pos = $sh{"index"};
    $sh{"tf"} = 1 - $sh{"tf"};
    if ($sh{"tf"} == 0) {
        $sh{"index"} += $sh{"chunk"};
    }
    return \%sh;
}

sub do_transform_binsrch($$$$) {
    my ($cfile, $tmpfile, $which, $state) = @_;
    my %sh = %{$state};

    my $instances = count_ifs($cfile, $which);
    my $tf = $sh{"tf"};
    my $index = $sh{"index"};
    my $chunk = $sh{"chunk"};

    open INF, "<$cfile" or die;
    open OUTF, ">$tmpfile" or die;

    return 2 if ($chunk <= 0);

    if ($index < $instances && $instances > 0) {
        my $ifs = -1;
        my $multiline = 0;
        # replace #if directives with #if 0 or #if 1
        while (my $line = <INF>) {
            my $newline = $line;

            if ($multiline) {
                if (not ($line =~ m/.*\\$/)) {
                    $multiline = 0;
                }
                next;
            }

            if ($line =~ m/^\s*#\s*if/) {
                $ifs++;
                if ($ifs >= $index && $ifs < $index + $chunk) {
                    $newline = "#if $tf\n";
                    if ($line =~ m/.*\\$/) {
                        $multiline = 1;
                    }
                }
            }
            print OUTF $newline;
        }
        close INF;
        close OUTF;

        # use unifdef to remove constant if blocks
        my $tmpfile2 = File::Temp::tmpnam();
        File::Copy::move($tmpfile, $tmpfile2);
        my $cmd = "$unifdef $options -k -o $tmpfile $tmpfile2 > /dev/null 2>&1";
        my $res = runit ($cmd);
        unlink $tmpfile2;

        # stop if unifdef returns error code
        return 2 if ($res == 2);

        my $new_instances = count_ifs($tmpfile, $which);
        print "went from $instances ifs to $new_instances with chunk ",
            "$chunk\n" if $DEBUG;
        return 0;
    }
    return 1;
}

sub transform ($$$) {
    my ($cfile, $which, $state) = @_;
    my %sh = %{$state};

    if (defined($sh{"start"})) {
        print "***TRANSFORM START***\n" if $DEBUG;
        delete $sh{"start"};
        $sh{"index"} = 0;
        $sh{"chunk"} = count_ifs($cfile, $which);
        $sh{"tf"} = 0;
    }

    my $tmpfile = File::Temp::tmpnam();

  AGAIN:
    my $res = do_transform_binsrch($cfile, $tmpfile, $which, \%sh);

    return ($STOP, \%sh) if ($res == 2);
    if ($res == 1) {
        # rechunk
        return ($STOP, \%sh) if ($sh{"chunk"} <= 1 && $sh{"tf"});
        my $newchunk = int ($sh{"chunk"} / 2.0);
        $sh{"chunk"} = $newchunk;
        print "granularity reduced to $newchunk\n" if $DEBUG;
        $sh{"index"} = 0;
        $sh{"tf"} = 0;
        goto AGAIN;
    }

    if (compare($cfile, $tmpfile) == 0) {
        print "did not change file\n" if $DEBUG;
        unlink $tmpfile;
        $sh{"tf"} = 1 - $sh{"tf"};
        $sh{"index"} += $sh{"chunk"} if (not $sh{"tf"});
        goto AGAIN;
    }

    File::Copy::move($tmpfile, $cfile);
    return ($OK, \%sh);
}

1;
