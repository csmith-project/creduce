## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2013, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_includes;

use strict;
use warnings;

use File::Copy;
use creduce_utils;

sub check_prereqs () {
    return 1;
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

sub do_transform($$) {
    my ($cfile, $index) = @_;

    open INF, "<$cfile" or die;
    my $tmpfile = File::Temp::tmpnam();
    open OUTF, ">$tmpfile" or die;

    my $includes = 0;
    my $matched;
    while (my $line = <INF>) {
        if ($line =~ m/^\s*#\s*include/) {
            $includes++;
            if ($includes == $index) {
                $matched = 1;
                next;
            }
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

    my $success;
    $success = do_transform($cfile, $index);
    return ($success ? $OK : $STOP, \$index);
}

1;
