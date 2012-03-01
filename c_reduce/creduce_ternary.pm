#!/usr/bin/perl -w

package creduce_ternary;

use Regexp::Common;
use re 'eval';
use Exporter::Lite;

@EXPORT      = qw(ternary1 ternary2);

use creduce_regexes;
use creduce_utils;

sub ternary1 ($$) {
    (my $oldfile, my $newfile, my $delta_pos) = @_;

    my $prog = read_file ($oldfile);
    return STOP if ($delta_pos > length($prog));

    my $first = substr($prog, 0, $delta_pos);
    my $rest = substr($prog, $delta_pos);
    if ($rest =~ s/^(?<del1>$borderorspc)(?<a>$varnumexp)\s*\?\s*(?<b>$varnumexp)\s*:\s*(?<c>$varnumexp)(?<del2>$borderorspc)/$+{del1}$+{b}$+{del2}/sm) {
	$prog = $first.$rest;
	my $n1 = "$+{del1}$+{a} ? $+{b} : $+{c}$+{del2}";
	my $n2 = "$+{del1}$+{b}$+{del2}";
	print "replacing $n1 with $n2\n" unless $QUIET;
	write_file ($prog, $newfile);
	return SUCCESS;
    }	    

    return FAILURE;
}

sub ternary2 ($$) {
    (my $oldfile, my $newfile, my $delta_pos) = @_;

    my $prog = read_file ($oldfile);
    return STOP if ($delta_pos > length($prog));

    $first = substr($prog, 0, $delta_pos);
    $rest = substr($prog, $delta_pos);
    if ($rest =~ s/^(?<del1>$borderorspc)(?<a>$varnumexp)\s*\?\s*(?<b>$varnumexp)\s*:\s*(?<c>$varnumexp)(?<del2>$borderorspc)/$+{del1}$+{c}$+{del2}/sm) {
	$prog = $first.$rest;
	my $n1 = "$+{del1}$+{a} ? $+{b} : $+{c}$+{del2}";
	my $n2 = "$+{del1}$+{c}$+{del2}";
	print "replacing $n1 with $n2\n" unless $QUIET;
	return SUCCESS;
    }

    return FAILURE;
}

1;
