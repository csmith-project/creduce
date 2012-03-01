#!/usr/bin/perl -w

package creduce_utils;

use Exporter::Lite;

@EXPORT      = qw(read_file write_file $SUCCESS $FAILURE $STOP);

$SUCCESS = 999;
$FAILURE = 1010;
$STOP = 1133;

sub read_file ($) {
    (my $cfile) = @_;
    open INF, "<$cfile" or die;
    my $prog = "";
    while (my $line = <INF>) {
	$prog .= $line;
    }
    if (substr($prog, 0, 1) ne " ") {
	$prog = " $prog";
    }
    if (substr ($prog, -1, 1) ne " ") {
	$prog = "$prog ";
    }
    close INF;
    return $prog;
}

sub write_file ($$) {
    (my $cfile, my $prog) = @_;
    open OUTF, ">$cfile" or die;
    print OUTF $prog;
    close OUTF;
}

1;
