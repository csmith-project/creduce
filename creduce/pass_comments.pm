## -*- mode: Perl -*-
##
## Copyright (c) 2015, 2016, 2019 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_comments;

use strict;
use warnings;

use File::Compare;
use creduce_utils;
use Regexp::Common qw /comment/;

sub check_prereqs () {
    return 1;
}

sub count_comments ($$) {
    my ($cfile, $which) = @_;
    open INF, "<$cfile" or die;
    my $n = 0;
    while (my $line = <INF>) {
        if ($line =~ m/\/\/(.*?)$/) {
            $n++;
        }
    }
    close INF;
    return $n;
}

sub new ($$) {
    my ($cfile, $which) = @_;
    my %sh;
    $sh{"pass"} = -2;
    $sh{"start"} = 1;
    return \%sh;
}

sub advance ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my %sh = %{$state};

    my $pass = $sh{"pass"};
    if ($pass < 0) {
        $sh{"pass"}++;
    } else {
        $sh{"index"} += $sh{"chunk"};
    }
    return \%sh;
}

sub do_transform_binsrch ($$$$) {
    my ($cfile, $tmpfile, $which, $state) = @_;
    my %sh = %{$state};

    my $instances = count_comments($cfile, $which);
    my $index = $sh{"index"};
    my $chunk = $sh{"chunk"};

    if ($index < $instances && $instances > 0) {
        open INF, "<$cfile" or die;
        open OUTF, ">$tmpfile" or die;
        my $comments = -1;
        while (my $line = <INF>) {
            my $newline = $line;
            if ($line =~ m/\/\/(.*?)$/) {
                $comments++;
                if ($comments >= $index && $comments < $index + $chunk) {
                    $newline =~ s/\/\/(.*?)$//g;
                }
            }
            print OUTF $newline;
        }
        close INF;
        close OUTF;

        my $new_instances = count_comments($tmpfile, $which);
        print "went from $instances comments to $new_instances ",
            "with chunk $chunk\n" if $DEBUG;

        return 0;
    }
    return 1;
}

sub transform ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my %sh = %{$state};

    my $prog = read_file ($cfile);
    my $prog2 = $prog;
    my $tmpfile = File::Temp::tmpnam();

  AGAIN:
    my $pass = $sh{"pass"};
    if ($pass == -2) {
        # remove all C-style comments
        $prog2 =~ s#/\*[^*]*\*+([^/*][^*]*\*+)*/|("(\\.|[^"\\])*"|'(\\.|[^'\\])*'|.[^/"'\\]*)#defined $2 ? $2 : ""#gse;
        write_file($tmpfile, $prog2);
    } elsif ($pass == -1) {
        # remove all C++-style comments
        $prog2 =~ s/\/\/(.*?)$//gm;
        write_file($tmpfile, $prog2);
    } elsif ($pass == 0) {
        # remove C++-style comments with binary search

        if (defined($sh{"start"})) {
            delete $sh{"start"};
            $sh{"index"} = 0;
            $sh{"chunk"} = count_comments($cfile, $which);
        }

        my $rechunk = do_transform_binsrch($cfile, $tmpfile, $which, \%sh);

        if ($rechunk) {
            return ($STOP, \%sh) if ($sh{"chunk"} <= 1);
            my $newchunk = int ($sh{"chunk"} / 2.0);
            $sh{"chunk"} = $newchunk;
            print "granularity reduced to $newchunk\n" if $DEBUG;
            $sh{"index"} = 0;
            goto AGAIN;
        }
    }

    if (compare($cfile, $tmpfile) == 0) {
        print "did not change file\n" if $DEBUG;
        unlink $tmpfile;
        if ($pass < 0) {
            $sh{"pass"}++;
        } else {
            $sh{"index"} += $sh{"chunk"};
        }
        goto AGAIN;
    }

    File::Copy::move($tmpfile, $cfile);
    return ($OK, \%sh);
}

1;
