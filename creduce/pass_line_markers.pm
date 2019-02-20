## -*- mode: Perl -*-
##
## Copyright (c) 2019 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_line_markers;

use strict;
use warnings;

use File::Copy;
use File::Compare;
use creduce_utils;

sub check_prereqs () {
    return 1;
}

sub count_line_markers ($$) {
    (my $cfile, my $which) = @_;
    open INF, "<$cfile" or die;
    my $n = 0;
    while (my $line = <INF>) {
        if ($line =~ m/^\s*#\s*[0-9]+/) {
            $n++;
        }
    }
    close INF;
    return $n;
}

sub new ($$) {
    my ($cfile, $which) = @_;
    my %sh;
    my $instances = count_line_markers($cfile, $which);
    $sh{"index"} = $instances;
    $sh{"chunk"} = $instances;
    return \%sh;
}

sub advance ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my %sh = %{$state};

    $sh{"index"} -= $sh{"chunk"};
    if ($DEBUG) {
        my $index = $sh{"index"};
        my $chunk = $sh{"chunk"};
        print "ADVANCE: index = $index, chunk = $chunk\n";
    }
    return \%sh;
}

sub transform ($$$) {
    my ($cfile, $which, $state) = @_;
    my %sh = %{$state};

  AGAIN:

    my $instances = count_line_markers($cfile, $which);
    $sh{"index"} = $instances if ($sh{"index"} > $instances);
    my $index = $sh{"index"};
    my $chunk = $sh{"chunk"};

    if ($index >= 0 && $instances > 0 && $chunk > 0) {
        my $start = $index - $chunk;
        $start = 0 if ($start < 0);

        open INF, "<$cfile" or die;
        my $tmpfile = File::Temp::tmpnam();
        open OUTF, ">$tmpfile" or die;
        my $line_markers = -1;
        while (my $line = <INF>) {
            if ($line =~ m/^\s*#\s*[0-9]+/) {
                $line_markers++;
                if ($line_markers >= $start && $line_markers < $index) {
                    next;
                }
            }
            print OUTF $line;
        }
        close INF;
        close OUTF;

        my $new_instances = count_line_markers($tmpfile, $which);
        print "went from $instances line markers to $new_instances ",
            "with chunk $chunk\n" if $DEBUG;

        if (compare($cfile, $tmpfile) == 0) {
            print "did not change file\n" if $DEBUG;
            unlink $tmpfile;
            $sh{"index"} -= $sh{"chunk"};
            goto AGAIN;
        }
        File::Copy::move($tmpfile, $cfile);
    } else {
        return ($STOP, \%sh) if ($chunk < 10);
        my $newchunk = int ($chunk / 2.0);
        $sh{"chunk"} = $newchunk;
        print "granularity reduced to $newchunk\n" if $DEBUG;
        $sh{"index"} = $instances;
        goto AGAIN;
    }

    return ($OK, \%sh);
}

1;
