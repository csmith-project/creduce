## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2013, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_lines;

use strict;
use warnings;

use POSIX;

use Cwd 'abs_path';
use File::Compare;

use creduce_config qw(bindir libexecdir);
use creduce_utils;

my $topformflat;

sub check_prereqs () {
    my $path;
    my $abs_bindir = abs_path(bindir);
    if ((defined $abs_bindir) && ($FindBin::RealBin eq $abs_bindir)) {
	# This script is in the installation directory.
	# Use the installed `topformflat'.
	$path = libexecdir . "/topformflat";
    } else {
	# Assume that this script is in the C-Reduce build tree.
	# Use the `topformflat' that is also in the build tree.
	$path = "$FindBin::Bin/../delta/topformflat";
    }
    if ((-e $path) && (-x $path)) {
	$topformflat = $path;
	return 1;
    }
    # Check Windows
    $path = $path . ".exe";
    if (($^O eq "MSWin32") && (-e $path) && (-x $path)) {
	$topformflat = $path;
	return 1;
    }
    return 0;
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
    die if (defined($sh{"start"}));
    my $pos = $sh{"index"};
    $sh{"index"} -= $sh{"chunk"};
    my $i = $sh{"index"};
    my $c = $sh{"chunk"};
    print "***ADVANCE*** from $pos to $i with chunk $c\n" if $DEBUG;
    return \%sh;
}

sub transform ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my %sh = %{$state};

    if (defined($sh{"start"})) {
	print "***TRANSFORM START***\n" if $DEBUG;
	delete $sh{"start"};
	my $outfile = File::Temp::tmpnam();
	my $cmd = qq{"$topformflat" $arg < $cfile > $outfile};
	print $cmd if $DEBUG;
	runit ($cmd);

	my $tmpfile = File::Temp::tmpnam();
	open INF_BLANK, "<$outfile" or die;
	open OUTF_BLANK, ">$tmpfile" or die;
	while (my $line = <INF_BLANK>) {
		if($line !~ /^\s*$/) {
			print OUTF_BLANK $line;
		}
	}
	close INF_BLANK;
	close OUTF_BLANK;

	if (compare($cfile, $tmpfile) == 0) {
	    # this is a gross hack to avoid tripping the
	    # pass-didn't-modify-file check in the C-Reduce core, in
	    # the (generally unlikely) case where topformflat didn't
	    # change the file at all
	    print "gross blank line hack!\n" if $DEBUG;
	    open OF, ">>$tmpfile" or die;
	    print OF "\n";
	    close OF;
	}
	File::Copy::move($tmpfile, $cfile);
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

    if ($DEBUG) {
	my $c = $sh{"chunk"};
	my $i = $sh{"index"};
	print "***TRANSFORM REGULAR chunk $c at $i***\n";
    }

    open INF, "<$cfile" or die;
    my @data = ();
    while (my $line = <INF>) {
	push @data, $line;
	if ($DEBUG && 0) {
	    chomp $line;
	    print "LINE PASS FILE DATA: '$line'\n";
	}
    }
    close INF;

  AGAIN:
    $sh{"index"} = scalar(@data) if ($sh{"index"} > scalar(@data));
    if ($sh{"index"} >= 0 && scalar(@data) > 0 && $sh{"chunk"} > 0) {
	my $start = $sh{"index"} - $sh{"chunk"};
	$start = 0 if ($start < 0);
	my $lines = scalar(@data);
	splice @data, $start, $sh{"chunk"};
	my $newlines = scalar(@data);
	my $c = $sh{"chunk"};
	print "went from $lines lines to $newlines with chunk $c\n" if $DEBUG;
	my $tmpfile = File::Temp::tmpnam();
	open OUTF, ">$tmpfile" or die;
	foreach my $line (@data) {
	    print OUTF $line;
	}
	close OUTF;
	if (compare($cfile, $tmpfile) == 0) {
	    print "did not change file\n" if $DEBUG;
	    unlink $tmpfile;
	    $sh{"index"} -= $sh{"chunk"};
	    goto AGAIN;
	}
	File::Copy::move($tmpfile, $cfile);
    } else {
	return ($STOP, \%sh) if ($sh{"chunk"} <= 1);
	my $newchunk = int ($sh{"chunk"} / 2.0);
	$sh{"chunk"} = $newchunk;
	print "granularity reduced to $newchunk\n" if $DEBUG;
	$sh{"index"} = scalar(@data);
	goto AGAIN;
    }

    return ($OK, \%sh);
}

1;
