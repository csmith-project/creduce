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

sub init () {
}

sub remove_outside ($) 
{
    (my $str) = @_;
    $str =~ s/^.(.*).$/$1/;
    return $str;
}

sub transform ($$$) {
    (my $cfile, my $index, my $arg) = @_;

    my $prog = read_file ($cfile);
    my $prog2 = $prog;

    $replace_cont = -1;

    if (0) {
    } elsif ($arg eq "parens") {
	$prog2 =~ s/(?<all>($RE{balanced}{-parens=>'()'}))/replace_aux($index,$+{all},"")/eg;
    } elsif ($arg eq "curly") {
	$prog2 =~ s/(?<all>($RE{balanced}{-parens=>'{}'}))/replace_aux($index,$+{all},"")/eg;
    } elsif ($arg eq "angles") {
	$prog2 =~ s/(?<all>($RE{balanced}{-parens=>'<>'}))/replace_aux($index,$+{all},"")/eg;
    } elsif ($arg eq "parens-only") {
	$prog2 =~ s/(?<all>($RE{balanced}{-parens=>'()'}))/replace_aux($index,$+{all},remove_outside($+{all}))/eg;
    } elsif ($arg eq "curly-only") {
	$prog2 =~ s/(?<all>($RE{balanced}{-parens=>'{}'}))/replace_aux($index,$+{all},remove_outside($+{all}))/eg;
    } elsif ($arg eq "angles-only") {
	$prog2 =~ s/(?<all>($RE{balanced}{-parens=>'<>'}))/replace_aux($index,$+{all},remove_outside($+{all}))/eg;
    } else {
	die "pass_balanced: expected arg to be parens, curly, or angles";
    }

    if ($prog ne $prog2) {
	write_file ($cfile, $prog2);
	return $SUCCESS;
    } else {
	return $STOP;
    }
}

1;
