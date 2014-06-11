#!/usr/bin/perl -w
##
## Copyright (c) 2013, 2014 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

use strict;

my $DIR = "XXDIR";

my $code;

sub desirable ($$$$) {
    (my $sizewin, my $warnwin, my $oratio, my $nratio) = @_;
    # return $warnwin > 0;
    return $nratio >= $oratio;
}

sub get_size ($) {
    (my $fn) = @_;
    open INF, "<$fn" or return 0;
    my $line = <INF>;
    chomp $line;
    if (!($line =~ /^([0-9]+)$/)) {
	print "no size in size file\n";
	return 0;
    }
    my $size = $1;
    close INF;
    print "read size of $size\n";
    return $size;
}

my $res = system "( ulimit -t 60 ; ulimit -v 16252928 ; clang++ -std=c++11 -c XXFILE.cpp -o /dev/null ) 2>&1 > /dev/null | wc -c > out.txt";
if ($res!=0) {
    $code = -1;
    goto EXIT;
}
if (! -f "$DIR/out_best.txt") {
    system "cp out.txt $DIR/out_best.txt";
    $code = 0;
    goto EXIT;
}

my $osize = -s "$DIR/XXFILE.best";
my $nsize = -s "XXFILE.cpp";
my $sizewin = $osize - $nsize;
print "old size = $osize, new = $nsize, win = $sizewin\n";

my $owarn = get_size ("$DIR/out_best.txt");
my $nwarn = get_size ("out.txt");
my $warnwin = $nwarn - $owarn;
print "old warn = $owarn, new = $nwarn, win = $warnwin\n";

my $oratio = (0.0+$owarn)/$osize;
my $nratio = (0.0+$nwarn)/$nsize;
print "old ratio = $oratio, new = $nratio\n";

if (desirable($sizewin,$warnwin,$oratio,$nratio)) {
    system "cp out.txt $DIR/out_best.txt";
    $code = 0;
    goto EXIT;
} else {
    $code = -1;
    goto EXIT;
}

EXIT:
#sleep (3600);
exit ($code);
