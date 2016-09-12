## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

# this pass is really dumb, but it does a lot of work and is hard to
# replace...

package pass_peep;

use strict;
use warnings;

use creduce_regexes;
use creduce_utils;

use Regexp::Common;
use re 'eval';

sub check_prereqs () {
    return 1;
}

sub new ($$) {
    my %sh;
    $sh{"index"} = 0;
    $sh{"index2"} = 0;
    return \%sh;
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

sub advance ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my %sh = %{$state};
    my $lim;
    if ($which eq "a") {
	$lim = scalar (@regexes_to_replace);
    } elsif ($which eq "b") {
	$lim = scalar (@delimited_regexes_to_replace);
    } elsif ($which eq "c") {
        $lim = 1;
    } else {
	die;
    }
    $sh{"index2"}++;
    if ($sh{"index2"} >= $lim) {
	$sh{"index2"} = 0;
	$sh{"index"}++;
    }
    return \%sh;
}

sub transform ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my %sh = %{$state};

    my $prog = read_file ($cfile);
    my $prog2 = $prog;

    while (1) {

	return ($STOP, \%sh) if ($sh{"index"} > length ($prog));

	if (0) {
	} elsif ($which eq "a") {
	    my $l = $regexes_to_replace[$sh{"index2"}];
	    my $str = @{$l}[0];
	    my $repl = @{$l}[1];
	    my $first = substr($prog2, 0, $sh{"index"});
	    my $rest = substr($prog2, $sh{"index"});
	    if ($rest =~ s/^($str)/$repl/sm) {
		$prog2 = $first.$rest;
		if ($prog ne $prog2) {
		    write_file ($cfile, $prog2);
		    return ($OK, \%sh);
		}
	    }
	} elsif ($which eq "b") {
	    my $l = $delimited_regexes_to_replace[$sh{"index2"}];
	    my $str = @{$l}[0];
	    my $repl = @{$l}[1];
	    my $first = substr($prog2, 0, $sh{"index"});
	    my $rest = substr($prog2, $sh{"index"});
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
		    return ($OK, \%sh);
		}
	    }
	} elsif ($which eq "c") {
	  my $index = $sh{"index"};
	  my $first = substr($prog2, 0, $index);
	  my $rest = substr($prog2, $index);

	  if ($rest =~ m/^while\s*$RE{balanced}{-parens=>'()'}\s*$RE{balanced}{-parens=>'{}'}/) {
	    my $expr = $1;
	    my $body = $2;
	    my $remain = substr($rest, $+[0]);
	    $body =~ s/break\s*;//g;
	    $prog2 = $first.$body.$remain;
	    if ($prog ne $prog2) {
	      write_file ($cfile, $prog2);
	      return ($OK, \%sh);
	    }
	  }
	} else {
	  die;
	}
      out:
	$state = advance($cfile, $which, \%sh);
	%sh = %{$state};
    }
}

1;
