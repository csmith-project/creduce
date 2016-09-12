## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_balanced;

use strict;
use warnings;

use Regexp::Common;
use re 'eval';

use creduce_regexes;
use creduce_utils;

sub check_prereqs () {
    return 1;
}

sub new ($$) {
    my $pos = 0;
    return \$pos;
}

sub advance ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my $pos = ${$state};
    $pos++;
    return \$pos;
}

sub remove_outside ($) {
    (my $str) = @_;
    substr($str,0,1) = "";
    substr($str,-1,1) = "";
    return $str;
}

# this function is idiotically stupid and slow but I spent a long time
# trying to get nested matches out of Perl's various utilities for
# matching balanced delimiters, with no success

sub transform ($$$) {
    (my $cfile, my $arg, my $state) = @_;

    my $pos = ${$state};
    my $prog = read_file ($cfile);

    while (1) {

	my $first = substr ($prog, 0, $pos);
	my $rest = substr ($prog, $pos);
	my $rest2 = $rest;

	if (0) {
	} elsif ($arg eq "square-inside") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'[]'}))/\[\]/s;
	} elsif ($arg eq "angles-inside") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'<>'}))/\<\>/s;
	} elsif ($arg eq "parens-inside") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'()'}))/\(\)/s;
	} elsif ($arg eq "curly-inside") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'{}'}))/\{\}/s;
	} elsif ($arg eq "square") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'[]'}))//s;
	} elsif ($arg eq "angles") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'<>'}))//s;
	} elsif ($arg eq "parens-to-zero") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'()'}))/0/s;
	} elsif ($arg eq "parens") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'()'}))//s;
	} elsif ($arg eq "curly") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'{}'}))//s;
	} elsif ($arg eq "curly2") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'{}'}))/;/s;
	} elsif ($arg eq "curly3") {
	    $rest2 =~ s/^(?<all>(=\s*$RE{balanced}{-parens=>'{}'}))//s;
	} elsif ($arg eq "parens-only") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'()'}))/remove_outside($+{all})/se;
	} elsif ($arg eq "curly-only") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'{}'}))/remove_outside($+{all})/se;
	} elsif ($arg eq "angles-only") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'<>'}))/remove_outside($+{all})/se;
	} elsif ($arg eq "square-only") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'[]'}))/remove_outside($+{all})/se;
	} else {
	    return ($ERROR, "unexpected argument");
	}
	if ($rest ne $rest2) {
	    write_file ($cfile, $first . $rest2);
	    return ($OK, \$pos);
	}
	$pos++;
	if ($pos > length($prog)) {
	    return ($STOP, \$pos);
	}
    }
}

1;
