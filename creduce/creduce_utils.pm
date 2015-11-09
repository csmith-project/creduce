## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2013, 2015 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package creduce_utils;

use warnings;

use Exporter::Lite;
use File::Spec;
use File::Which;

@EXPORT      = qw($DEBUG $IGNORE_PASS_BUGS $OK $STOP $ERROR
		  find_external_program
		  runit 
                  run_clang_delta
		  $replace_cont $matched replace_aux
		  read_file write_file
                  );

$DEBUG = 0;
$IGNORE_PASS_BUGS = 0;

$OK = 999999;
$STOP = 111333;
$ERROR = 223334;

sub find_external_program($$) {
    my ($configured_path, $program_name) = @_;
    # $configured_path is the pathname that was found at configure time.
    # If it seems OK, use it.
    # Otherwise, try looking in the user's path for $program_name.
    if (File::Spec->file_name_is_absolute ($configured_path)
	&& -e $configured_path
	&& -f $configured_path
	&& -x $configured_path) {
	return $configured_path;
    }
    return File::Which::which ($program_name);
}

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

sub run_clang_delta ($) {
    (my $cmd) = @_;
    if ((system "$cmd") != 0) {
        my $res = $? >> 8;
        if ($res == 255) {
            return -1;
        }
        elsif ($res == 1) {
            return -2;
        }
        else {
            return -3;
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
