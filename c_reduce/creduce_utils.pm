#!/usr/bin/perl -w

package creduce_utils;

use Exporter::Lite;

@EXPORT      = qw(read_file write_file $SUCCESS $FAILURE $STOP
                  $replace_cont replace_aux);

$SUCCESS = 999;
$FAILURE = 1010;
$STOP = 1133;

$replace_cont = 0;
sub replace_aux ($$$) { 
    my ($index,$original,$replacement) = @_;
    $replace_cont++;
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
