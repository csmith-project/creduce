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

sub init () {
    $index = 0;
}

sub advance () {
    $index++;
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

sub transform ($$) {
    (my $cfile, my $which) = @_;

    my $prog = read_file ($cfile);
    my $prog2 = $prog;

    while (1) {
	
	return $STOP if ($index > length ($prog));
	
	if (0) {
	} elsif ($which eq "a") {
	    my $n=-1;
	    foreach my $l (@regexes_to_replace) {	       
		$n++;
		my $str = @{$l}[0];
		my $repl = @{$l}[1];
		my $first = substr($prog2, 0, $index);
		my $rest = substr($prog2, $index);
		my $rrest = $rest;
		if ($rest =~ s/^($str)/$repl/sm) {
		    my $before = $1;
		    my $zz1 = $rest;
		    my $zz2 = $rrest;
		    ($zz1 =~ s/\s//g);
		    ($zz2 =~ s/\s//g);
		    if ($zz1 ne $zz2) {			
			$prog2 = $first.$rest;
			write_file ($cfile, $prog2);
			return $SUCCESS;
		    }
		}
	    }
	} elsif ($which eq "b") {
	    my $n=-1;
	    foreach my $l (@delimited_regexes_to_replace) {
		$n++;
		my $str = @{$l}[0];
		my $repl = @{$l}[1];
		my $first = substr($prog2, 0, $index);
		my $rest = substr($prog2, $index);
		
		my $rrest = $rest;
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
		next if ($repl eq "0" && $rest =~ /^($front)0$back/sm);
		next if ($repl eq "1" && $rest =~ /^($front)0$back/sm);
		next if ($repl eq "1" && $rest =~ /^($front)1$back/sm);
		next if ($repl =~ /0\s*,/ && $rest =~ /^($front)0\s*,$back/sm);
		next if ($repl =~ /1\s*,/ && $rest =~ /^($front)0\s*,$back/sm);
		next if ($repl =~ /1\s*,/ && $rest =~ /^($front)1\s*,$back/sm);
		next if ($repl =~ /,\s*0/ && $rest =~ /^($front),\s*0$back/sm);
		next if ($repl =~ /,\s*1/ && $rest =~ /^($front),\s*0$back/sm);
		next if ($repl =~ /,\s*1/ && $rest =~ /^($front),\s*1$back/sm);
		
		if (
		    $rest =~ s/^$front(?<str>$str)$back/$+{delim1}$repl$+{delim2}/sm
		    ) {
		    my $before = $+{str};
		    my $zz1 = $rest;
		    my $zz2 = $rrest;
		    ($zz1 =~ s/\s//g);
		    ($zz2 =~ s/\s//g);
		    if ($zz1 ne $zz2) {
			$prog2 = $first.$rest;
			write_file ($cfile, $prog2);
			return $SUCCESS;
		    } else {
			# die;
		    }
		}
	    }
	}
	$index++;
    }
}

1;
