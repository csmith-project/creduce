## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2013, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_blank;

use strict;
use warnings;

use File::Copy;
use creduce_utils;

my @rx = (qr/^\s*$/, qr/^#/);

sub check_prereqs () {
    return 1;
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

sub do_transform($$) {
    my ($cfile, $pattern) = @_;

    open INF, "<$cfile" or die;
    my $tmpfile = File::Temp::tmpnam();
    open OUTF, ">$tmpfile" or die;

    my $matched;
    while (my $line = <INF>) {
        if ($line =~ $pattern) {
            $matched = 1;
            next;
        }
        print OUTF $line;
    }

    close INF;
    close OUTF;
    if ($matched) {
        File::Copy::move($tmpfile, $cfile);
    } else {
        unlink $tmpfile;
    }
    return $matched;
}

sub transform ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my $index = ${$state};

    return ($STOP, \$index) unless ($index < scalar @rx);

    my $success;
    until ($success || $index >= scalar @rx) {
        $success = do_transform($cfile, $rx[$index]);
        $index++;
    }
    return ($success ? $OK : $STOP, \$index);
}

1;
