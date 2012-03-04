#!/usr/bin/perl -w

package pass_balanced;

use Regexp::Common;
use re 'eval';

use strict;

use creduce_regexes;
use creduce_utils;

sub check_prereqs () {
    return 1;
}

my $pos;

sub init () {
    $pos = 0;
}

sub advance () {
    $pos++;
}

sub remove_outside ($) 
{
    (my $str) = @_;

    # sanity check
    die unless (substr($str,0,1) =~ /[\{\<\(]/);
    die unless (substr($str,-1,1) =~ /[\}\>\)]/);

    substr($str,0,1) = "";
    substr($str,-1,1) = "";
    return $str;
}

# this function is idiotically stupid and slow but I spent a long time
# trying to get nested matches out of Perl's various utilities for
# matching balanced delimiters, with no success

sub transform ($$) {
    (my $cfile, my $arg) = @_;

    my $prog = read_file ($cfile);

    while (1) {

	my $first = substr ($prog, 0, $pos);
	my $rest = substr ($prog, $pos);
	my $rest2 = $rest;
	
	if (0) {
	} elsif ($arg eq "parens") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'()'}))//s;
	} elsif ($arg eq "curly") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'{}'}))//s;
	} elsif ($arg eq "angles") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'<>'}))//s;
	} elsif ($arg eq "parens-only") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'()'}))/remove_outside($+{all})/se;
	} elsif ($arg eq "curly-only") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'{}'}))/remove_outside($+{all})/se;
	} elsif ($arg eq "angles-only") {
	    $rest2 =~ s/^(?<all>($RE{balanced}{-parens=>'<>'}))/remove_outside($+{all})/se;
	} else {
	    die "pass_balanced: expected arg to be parens, curly, or angles";
	}
	if ($rest ne $rest2) {
	    my $prog2 = $first.$rest2;
	    write_file ($cfile, $prog2);
	    return $SUCCESS;
	}
	$pos++;
	if ($pos > length($prog)) {
	    return $STOP;
	}
    }
}

1;
