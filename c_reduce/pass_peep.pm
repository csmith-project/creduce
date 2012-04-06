#!/usr/bin/perl -w

package pass_peep;

use strict;

use creduce_regexes;
use creduce_utils;
use re 'eval';

sub check_prereqs () {
    return 1;
}

my $index;
my $index2;

sub reset ($$) {
    $index = 0;
    $index2 = 0;
}

my @subexprs = (
    "($fullvar)(\\s*)($binop)(\\s*)($fullvar)",
    "($fullvar)(\\s*)($binop)",
    "($binop)(\\s*)($fullvar)",
    "($fullvar)",
    ":(\\s*)($fullvar)",
    "::(\\s*)($fullvar)",
    "($fullvar)(\\s*):",
    "($fullvar)(\\s*)::",
    "($fullvar)(\\s*\\?\\s*)($fullvar)(\\s*\\:\\s*)($fullvar)",
    );

foreach my $x (@subexprs) {
    push @delimited_regexes_to_replace, ["$x", "0"];
    push @delimited_regexes_to_replace, ["$x", "1"];
    push @delimited_regexes_to_replace, ["$x", ""];
    push @delimited_regexes_to_replace, ["$x\\s*,", "0,"];
    push @delimited_regexes_to_replace, ["$x\\s*,", "1,"];
    push @delimited_regexes_to_replace, ["$x\\s*,", ""];
    push @delimited_regexes_to_replace, [",\\s*$x", ",0"];
    push @delimited_regexes_to_replace, [",\\s*$x", ",1"];
    push @delimited_regexes_to_replace, [",\\s*$x", ""];
}

sub advance ($) {
    (my $which) = @_;
    my $lim;
    if ($which eq "a") {
	$lim = scalar (@regexes_to_replace);
    } elsif ($which eq "b") {
	$lim = scalar (@delimited_regexes_to_replace);
    } else {
	die;
    }
    $index2++;
    if ($index2 >= $lim) {
	$index2 = 0;
	$index++;
    } 
}

sub transform ($$) {
    (my $cfile, my $which) = @_;

    my $prog = read_file ($cfile);
    my $prog2 = $prog;

    while (1) {
	
	return $STOP if ($index > length ($prog));
	
	if (0) {
	} elsif ($which eq "a") {
	    my $l = $regexes_to_replace[$index2];
	    my $str = @{$l}[0];
	    my $repl = @{$l}[1];
	    my $first = substr($prog2, 0, $index);
	    my $rest = substr($prog2, $index);
	    if ($rest =~ s/^($str)/$repl/sm) {
		$prog2 = $first.$rest;
		if ($prog ne $prog2) {
		    write_file ($cfile, $prog2);
		    return $OK;
		}
	    }
	} elsif ($which eq "b") {
	    my $l = $delimited_regexes_to_replace[$index2];
	    my $str = @{$l}[0];
	    my $repl = @{$l}[1];
	    my $first = substr($prog2, 0, $index);
	    my $rest = substr($prog2, $index);
	    my $front;
	    my $back;
	    if (substr($rest,0,1) eq ",") {
		$front = "(?<delim1>($borderorspc)?)";
	    } else {
		$front = "(?<delim1>$borderorspc)";
	    }
	    if (substr($rest,-1,1) eq ",") {
		$back = "(?<delim2>($borderorspc)?)";
	    } else {
		$back = "(?<delim2>$borderorspc)";
	    }
	    
	    # special cases to avoid infinite replacement loops
	    goto out if ($repl eq "0" && $rest =~ /^($front)0$back/sm);
	    goto out if ($repl eq "1" && $rest =~ /^($front)0$back/sm);
	    goto out if ($repl eq "1" && $rest =~ /^($front)1$back/sm);
	    goto out if ($repl =~ /0\s*,/ && $rest =~ /^($front)0\s*,$back/sm);
	    goto out if ($repl =~ /1\s*,/ && $rest =~ /^($front)0\s*,$back/sm);
	    goto out if ($repl =~ /1\s*,/ && $rest =~ /^($front)1\s*,$back/sm);
	    goto out if ($repl =~ /,\s*0/ && $rest =~ /^($front),\s*0$back/sm);
	    goto out if ($repl =~ /,\s*1/ && $rest =~ /^($front),\s*0$back/sm);
	    goto out if ($repl =~ /,\s*1/ && $rest =~ /^($front),\s*1$back/sm);
	    
	    if ($rest =~ s/^$front(?<str>$str)$back/$+{delim1}$repl$+{delim2}/sm) {
		$prog2 = $first.$rest;
		if ($prog ne $prog2) {
		    write_file ($cfile, $prog2);
		    return $OK;
		} 
	    }
	} else {
	    die;
	}
      out:
	advance($which);
    }
}

1;
