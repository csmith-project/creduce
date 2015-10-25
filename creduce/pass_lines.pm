## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2013, 2015 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_lines;

use strict;
use warnings;

use POSIX;

use creduce_config qw(TOPFORMFLAT);
use creduce_utils;

my $topformflat;

sub check_prereqs () {
    $topformflat = find_external_program(creduce_config::TOPFORMFLAT,
					 "topformflat");
    return defined($topformflat);
}

# unlike the previous version of pass_lines, this one always
# progresses from the back of the file to the front

sub new ($$) {
    (my $cfile, my $arg) = @_;
    my %sh;
    $sh{"start"} = 1;
    return \%sh;
}

sub advance ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my %sh = %{$state};

    return \%sh if defined($sh{"start"});

    my $pos = $sh{"index"};
    $sh{"index"} -= $sh{"chunk"};
    my $i = $sh{"index"};
    my $c = $sh{"chunk"};
    print "advance from $pos to $i with chunk $c\n" if $VERBOSE;
    return \%sh;
}

sub transform ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my %sh = %{$state};

    if (defined($sh{"start"})) {
	delete $sh{"start"};
	my $tmpfile = File::Temp::tmpnam();
	system "$topformflat $arg < $cfile > $tmpfile";
	File::Copy::move($tmpfile, $cfile);
	print "ran $topformflat $arg < $cfile > $tmpfile\n" if $VERBOSE;
	open INF, "<$cfile" or die;
	my @data = ();
	while (my $line = <INF>) {
	    push @data, $line;
	}
	close INF;
	my $l = scalar(@data);
	$sh{"index"} = $l;
	$sh{"chunk"} = $l;
	return ($OK, \%sh);
    }

    open INF, "<$cfile" or die;
    my @data = ();
    while (my $line = <INF>) {
	push @data, $line;
    }
    close INF;

  AGAIN:
    # don't bother unless there's at least half a chunk
    if ($sh{"index"} > ($sh{"chunk"} / 2)) {
	my $start = $sh{"index"} - $sh{"chunk"};
	$start = 0 if ($start < 0);
	my $lines = scalar(@data);
	splice @data, $start, $sh{"chunk"};
	my $newlines = scalar(@data);
	my $c = $sh{"chunk"};
	print "went from $lines lines to $newlines with chunk $c\n" if $VERBOSE;
	# if we fell off the end we're done
	if ($newlines >= $lines) {
	    $sh{"index"} -= $sh{"chunk"};
	    goto AGAIN;
	}
	open OUTF, ">$cfile" or die;
	foreach my $line (@data) {
	    print OUTF $line;
	}
	close OUTF;
    } else {
	return ($STOP, \%sh) if ($sh{"chunk"} <= 1);
	my $newchunk = int ($sh{"chunk"} / 2.0);
	$sh{"chunk"} = $newchunk;
	print "granularity reduced to $newchunk\n" if $VERBOSE;
	$sh{"index"} = scalar(@data);
	goto AGAIN;
    }

    return ($OK, \%sh);
}

1;
