## -*- mode: Perl -*-
##
## Copyright (c) 2012 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package creduce_utils;

use warnings;

use Exporter::Lite;

@EXPORT      = qw(read_file write_file $OK $STOP $VERBOSE
                  $replace_cont replace_aux runit $matched);

$VERBOSE = 0;

$OK = 999999;
$STOP = 111333;

sub runit ($) {
    (my $cmd) = @_;
    if ((system "$cmd") != 0) {
        my $res = $? >> 8;
        if ($res == 255) {
            return -1;
        }
        else {
            return -2;
        }
    }
    return ($? >> 8);
}

# utility code to help us replace the nth occurrence of a pattern
$replace_cont = 0;
$matched = 0;
sub replace_aux ($$$) { 
    my ($index,$original,$replacement) = @_;
    $replace_cont++;
    $matched = 1;
    return ($replace_cont == $index) ? $replacement : $original;
}

sub read_file ($) {
    (my $cfile) = @_;
    open INF, "<$cfile" or die;
    my $prog = "";
    while (my $line = <INF>) {
	$prog .= $line;
    }
    close INF;
    if (substr($prog, 0, 1) ne " ") {
	$prog = " $prog";
    }
    if (substr ($prog, -1, 1) ne " ") {
	$prog = "$prog ";
    }
    return $prog;
}

sub write_file ($$) {
    (my $cfile, my $prog) = @_;
    open OUTF, ">$cfile" or die;
    print OUTF $prog;
    close OUTF;
}

1;
