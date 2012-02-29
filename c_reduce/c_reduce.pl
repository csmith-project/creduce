#!/usr/bin/perl -w

######################################################################
#
# This Delta debugger specifically targets C/C++ code. Its design
# point is to be slow, thorough, and stupid. We count on the delta
# test script being able to rapidly discard syntactically invalid
# delta variants (for example, by compiling them).
#
####################################################################

# sudo apt-get install libfile-which-perl libregexp-common-perl indent astyle delta

# TODO:

# by default, exit if one of the tools we want isn't available
# put regex passses in a perl module
# make passes explicitly load and store the file
# add sanity check for clang_delta: if it returns success, the
#   file needs to have changed, and it still needs to compile
# make pass 0 use a hand-tuned phase ordering, after that it doesn't matter
#   use cleanup passes often
#   don't call variable combiner
# disable sanity check and enable quiet mode for timing runs
# make this script follow the interface from the paper
#   fully decouple delta_pos from file position
# add a pass to turn hex constants into decimal
# add an option to keep stats about fast vs. slow tests
# expose quiet on command line
# add an API for creating temporary files
# add an option limiting the number of passes
# see if it's faster to work from back to front
# watch for unexpected abnormal compiler outputs
# exploit early-exit from delta test to speed this up
#   keep per-pass statistic on the probability of requiring the slow test
#   invert this to decide how many fast tests to run in a row
#   need to keep checkpoints

######################################################################

use strict;
use Regexp::Common;
use re 'eval';
use File::Which;

######################################################################

# if set, save all delta variants
my $DEBUG = 0;

# if set, ensure the delta test succeeds before starting each pass
my $SANITY = 1;

my $QUIET = 0;

######################################################################

my $prog;
my $orig_prog_len;

sub print_pct () {
    my $pct = 100 - (length($prog)*100.0/$orig_prog_len);
    printf "(%.1f %%)\n", $pct;
}

sub find_match ($$$) {
    (my $p2, my $s1, my $s2) = @_;
    my $count = 1;
    die if (!(defined($p2) && defined($s1) && defined($s2)));
    while ($count > 0) {
	return -1 if ($p2 >= (length ($prog)-1));
	my $s = substr($prog, $p2, 1);
	if (!defined($s)) {
	    my $l = length ($prog);
	    print "$p2 $l\n";
	    die;
	}
	$count++ if ($s eq $s1);
	$count-- if ($s eq $s2);
	$p2++;
    }
    return $p2-1;
}

# these are set at startup time and never change
my $cfile;
my $test;
my $trial_num = 0;   

sub read_file_helper () {
    open INF, "<$cfile" or die;
    my $xxprog = "";
    while (my $line = <INF>) {
	$xxprog .= $line;
    }
    if (substr($xxprog, 0, 1) ne " ") {
	$xxprog = " $xxprog";
    }
    if (substr ($xxprog, -1, 1) ne " ") {
	$xxprog = "$xxprog ";
    }
    close INF;
    return $xxprog;
}

sub read_file () {
    $prog = read_file_helper();
}

sub count_lines () {
    open INF, "<$cfile" or die;
    my $n=0;
    $n++ while (<INF>);
    close INF;
    return $n;
}

sub ensure_mem_and_disk_are_synced () {
    die unless ($prog eq read_file_helper());
}

sub write_file () {
    $trial_num++;
    open OUTF, ">$cfile" or die;
    print OUTF $prog;
    close OUTF;
}

sub runit ($) {
    (my $cmd) = @_;
    if ((system "$cmd") != 0) {
	return -1;
    }   
    return ($? >> 8);
}

sub run_test () {
    my $res = runit "./$test >/dev/null 2>&1";
    return ($res == 0);
}

my $good_cnt;
my $bad_cnt;
my $pass_num = 0;
my $delta_pos;
my %method_worked = ();
my %method_failed = ();
my $old_size = 1000000000;

sub sanity_check () {
    print "sanity check... " unless $QUIET;
    my $res = run_test ();
    if (!$res) {
	die "test (and sanity check) fails";
    }
    print "successful\n" unless $QUIET;
}

sub delta_step_fail ($) {
    (my $method) = @_;
    print "failure\n" unless $QUIET;
    system "cp $cfile.bak $cfile";
    read_file();
    $bad_cnt++;
    $method_failed{$method}++;
}
    
my $changed_on_disk = 0;
my $delta_method;
my %cache = ();
my $cache_hits = 0;

sub delta_test () {
    if ($changed_on_disk) {
	read_file();
	$changed_on_disk = 0;
    } else {
	write_file();
    }

    # sanity check that could be deleted
    ensure_mem_and_disk_are_synced();

    my $len = length ($prog);
    print "[$pass_num $delta_method ($delta_pos / $len) s:$good_cnt f:$bad_cnt] " 
	unless $QUIET;

    my $result = $cache{$len}{$prog};

    if (defined($result)) {
	$cache_hits++;
	print "(hit) " unless $QUIET;
    } else {    
	$result = run_test ();
	$cache{$len}{$prog} = $result;
    }
    
    if ($result) {
	print "success " unless $QUIET;
	print_pct();
	system "cp $cfile $cfile.bak";
	$good_cnt++;
	$method_worked{$delta_method}++;
	my $size = length ($prog);
	if ($size < $old_size) {
	    foreach my $k (keys %cache) {
		if ($k > ($size + 5000)) {
		    $cache{$k} = ();
		}
	    }
	}
	$old_size = $size;
	return 1;
    } else {
	delta_step_fail($delta_method);
	return 0;
    }
}

############################## begin delta passes #############################

my $exit_delta_pass;
my $delta_worked;

sub lines ($) {
    (my $chunk_size) = @_;

    my $chunk_start = $delta_pos * $chunk_size;

    open INF, "<$cfile" or die;
    open OUTF, ">tmpfile" or die;

    my $n=0;
    my $did_something=0;

    while (my $line = <INF>) {
	if ($n < $chunk_start ||
	    $n >= ($chunk_start + $chunk_size)) {
	    print OUTF $line;
	} else {
	    chomp $line;
	    # TMI
	    # print "omitting: '$line'\n";
	    $did_something = 1;
	}
	$n++;
    }
    close INF;
    close OUTF;
    if ($did_something) {
	system "mv tmpfile $cfile";
	$changed_on_disk = 1;
	$delta_worked |= delta_test ();
    } else {
	$exit_delta_pass = 1;
    }
}

my $varnum = "(\\-?|\\+?)[0-9a-zA-Z\_]+";
my $varnumexp = "($varnum)|($RE{balanced}{-parens=>'()'})";
my $field = "\\.($varnum)";
my $index = "\\\[($varnum)\\\]";
my $fullvar = "([\\&\\*]*)($varnumexp)(($field)|($index))*";
my $arith = "\\+|\\-|\\%|\\/|\\*";
my $comp = "\\<\\=|\\>\\=|\\<|\\>|\\=\\=|\\!\\=|\\=";
my $logic = "\\&\\&|\\|\\|";
my $bit = "\\||\\&|\\^|\\<\\<|\\>\\>";
my $binop = "($arith)|($comp)|($logic)|($bit)|(\\-\\>)";
my $border = "[\\*\\{\\(\\[\\:\\,\\}\\)\\]\\;\\,]";
my $borderorspc = "(($border)|(\\s))";
my $fname = "(?<fname>$varnum)";
my $call = "$varnum\\s*$RE{balanced}{-parens=>'()'}";

# these match without additional qualification
my @regexes_to_replace = (
    ["$RE{balanced}{-parens=>'<>'}", ""],
    ["$RE{balanced}{-parens=>'()'}", ""],
    ["$RE{balanced}{-parens=>'{}'}", ""],
    ["namespace(.*?)$RE{balanced}{-parens=>'{}'}", ""],
    ["=\\s*$RE{balanced}{-parens=>'{}'}", ""],
    ["\\:\\s*[0-9]+\\s*;", ";"],
    ["\\;", ""],
    ["\#(.*?)\n", ""],
    ["\\^\\=", "="],
    ["\\|\\=", "="],
    ["\\&\\=", "="],
    ["\"(.*)\"", ""],
    ["\'(.*)\'", ""],
    ["\\+\\=", "="],
    ["\\-\\=", "="],
    ["\\*\\=", "="],
    ["\\/\\=", "="],
    ["\\%\\=", "="],
    ["\\<\\<\\=", "="],
    ["\\>\\>\\=", "="],
    ["\\+", ""],
    ["\\-", ""],
    [":", ""],
    [",", ""],
    ["::", ""],
    ["\\!", ""],
    ["\\~", ""],
    ["while", "if"],
    ['"(.*?)"', ""],
    ['"(.*?)",', ""],
    ["struct\\s*$RE{balanced}{-parens=>'{}'}", ""],
    ["union\\s*$RE{balanced}{-parens=>'{}'}", ""],
    ["enum\\s*$RE{balanced}{-parens=>'{}'}", ""],
    ["if\\s*$RE{balanced}{-parens=>'()'}", ""],
    );

# these match when preceded and followed by $borderorspc
my @delimited_regexes_to_replace = (
    ["($varnumexp)\\s*:", ""],
    ["goto\\s+($varnum);", ""],
    ["char", "int"],
    ["short", "int"],
    ["long", "int"],
    ["signed", "int"],
    ["unsigned", "int"],
    ["int(\\s+)argc(\\s*),(\\s*)(.*)(\\s*)\\*argv\\[\\]", "void"],
    ["int(\\s+)argc(\\s*),(\\s*)(.*)(\\s*)\\*(\\s*)\\*argv", "void"],
    ["int.*?;", ""],
    ["for", ""],
    ["\"(.*)\"", ""],
    ["\'(.*)\'", ""],
    ["\"(.*?)\"", ""],
    ["\'(.*?)\'", ""],
    ["$call,", "0"],
    ["$call,", ""],
    ["$call", "0"],
    ["$call", ""],
    );

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

my %regex_worked;
my %regex_failed;
my %delimited_regex_worked;
my %delimited_regex_failed;
for (my $n=0; $n<scalar(@regexes_to_replace); $n++) {
    $regex_worked{$n} = 0;
    $regex_failed{$n} = 0;
}
for (my $n=0; $n<scalar(@delimited_regexes_to_replace); $n++) {
    $delimited_regex_worked{$n} = 0;
    $delimited_regex_failed{$n} = 0;
}

sub replace_regex (){ 
    if (1) {
	my $n=-1;
	foreach my $l (@regexes_to_replace) {	       
	    $n++;
	    my $str = @{$l}[0];
	    my $repl = @{$l}[1];
	    my $first = substr($prog, 0, $delta_pos);
	    my $rest = substr($prog, $delta_pos);
	    my $rrest = $rest;
	    if ($rest =~ s/^($str)/$repl/sm) {
		my $before = $1;
		my $zz1 = $rest;
		my $zz2 = $rrest;
		($zz1 =~ s/\s//g);
		($zz2 =~ s/\s//g);
		if ($zz1 ne $zz2) {
		    print "regex $n replacing '$before' with '$repl' : " unless $QUIET;
		    $prog = $first.$rest;
		    if (delta_test ()) {
			$delta_worked = 1;
			$regex_worked{$n}++;
		    } else {
			$regex_failed{$n}++;
		    }
		}
	    }
	}
    }
    if (1) {
	my $n=-1;
	foreach my $l (@delimited_regexes_to_replace) {
	    $n++;
	    my $str = @{$l}[0];
	    my $repl = @{$l}[1];
	    my $first = substr($prog, 0, $delta_pos);
	    my $rest = substr($prog, $delta_pos);

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
		    print "regex $n delimited replacing '$before' with '$repl' : " unless $QUIET;
		    $prog = $first.$rest;
		    if (delta_test ()) {
			$delta_worked = 1;
			$delimited_regex_worked{$n}++;
		    } else {
			$delimited_regex_failed{$n}++;
		    }
		} else {
		    # die;
		}
	    }
	}
    }
}

sub blanks () {
    my $first = substr($prog, 0, $delta_pos);
    my $rest = substr($prog, $delta_pos);
    if ($rest =~ s/^(\s\t){2,}/ /) {
	$prog = $first.$rest;
	$delta_worked |= delta_test ();
    }
}

sub clang_delta ($) {
    (my $how) = @_;
    my $x = $delta_pos+1;
    my $cmd = "clang_delta --transformation=$how --counter=$x $cfile > foo";
    my $res = runit ($cmd);
    if ($res==0) {
	my $res2 = runit ("diff $cfile foo");
	if ($res2 == 0) {
	    die "oops, command '$cmd' produced identical output";
	}
	system "mv foo $cfile";
	$res2 = ("clang -O0 -c $cfile");
	if ($res != 0) {
	    die "oops couldn't compile $cfile now";
	}
	$changed_on_disk = 1;
	$delta_worked |= delta_test ();
    } else {
	$exit_delta_pass = 1;
    }
}

my $INDENT_OPTS = "-nbad -nbap -nbbb -cs -pcs -prs -saf -sai -saw -sob -ss ";

sub indent () {
    system "indent $INDENT_OPTS $cfile";
    $changed_on_disk = 1;
    $delta_worked |= delta_test ();
    $exit_delta_pass = 1;
}

sub final_indent () {
    system "indent -nbad -nbap -nbbb $cfile";
    system "astyle -A2 -xd -s2 $cfile";
    $changed_on_disk = 1;
    $delta_worked |= delta_test ();
    $exit_delta_pass = 1;
}

sub crc () {
    my $first = substr($prog, 0, $delta_pos);
    my $rest = substr($prog, $delta_pos);
    if ($rest =~ /^(?<all>transparent_crc\s*\((?<list>.*?)\))/) {
	my @stuff = split /,/, $+{list};
	my $var = $stuff[0];
	my $repl = "printf (\"%d\\n\", (int)$var)";
	print "crc call: < $+{all} > => < $repl > " unless $QUIET;
	substr ($rest, 0, length ($+{all})) = $repl;
	$prog = $first.$rest;
	$delta_worked |= delta_test ();
    }
}

sub ternary () {
    my $first = substr($prog, 0, $delta_pos);
    my $rest = substr($prog, $delta_pos);
    if ($rest =~ s/^(?<del1>$borderorspc)(?<a>$varnumexp)\s*\?\s*(?<b>$varnumexp)\s*:\s*(?<c>$varnumexp)(?<del2>$borderorspc)/$+{del1}$+{b}$+{del2}/sm) {
	$prog = $first.$rest;
	my $n1 = "$+{del1}$+{a} ? $+{b} : $+{c}$+{del2}";
	my $n2 = "$+{del1}$+{b}$+{del2}";
	print "replacing $n1 with $n2\n" unless $QUIET;
	$delta_worked |= delta_test ();
    }	    
    $first = substr($prog, 0, $delta_pos);
    $rest = substr($prog, $delta_pos);
    if ($rest =~ s/^(?<del1>$borderorspc)(?<a>$varnumexp)\s*\?\s*(?<b>$varnumexp)\s*:\s*(?<c>$varnumexp)(?<del2>$borderorspc)/$+{del1}$+{c}$+{del2}/sm) {
	$prog = $first.$rest;
	my $n1 = "$+{del1}$+{a} ? $+{b} : $+{c}$+{del2}";
	my $n2 = "$+{del1}$+{c}$+{del2}";
	print "replacing $n1 with $n2\n" unless $QUIET;
	$delta_worked |= delta_test ();
    }
}

sub shorten_ints () {
    my $first = substr($prog, 0, $delta_pos);
    my $rest = substr($prog, $delta_pos);
    if ($rest =~ s/^(?<pref>$borderorspc(\\-|\\+)?(0|(0[xX]))?)(?<del>[0-9a-fA-F])(?<numpart>[0-9a-fA-F]+)(?<suf>[ULul]*$borderorspc)/$+{pref}$+{numpart}$+{suf}/sm) {
	$prog = $first.$rest;
	my $n1 = "$+{pref}$+{del}$+{numpart}$+{suf}";
	my $n2 = "$+{pref}$+{numpart}$+{suf}";
	print "replacing $n1 with $n2\n" unless $QUIET;
	$delta_worked |= delta_test ();
    }      
    $first = substr($prog, 0, $delta_pos);
    $rest = substr($prog, $delta_pos);
    my $orig_rest = $rest;
    if ($rest =~ s/^(?<pref1>$borderorspc)(?<pref2>(\\-|\\+)?(0|(0[xX]))?)(?<numpart>[0-9a-fA-F]+)(?<suf>[ULul]*$borderorspc)/$+{pref1}$+{numpart}$+{suf}/sm && ($rest ne $orig_rest)) {
	$prog = $first.$rest;
	my $n1 = "$+{pref1}$+{pref2}$+{numpart}$+{suf}";
	my $n2 = "$+{pref1}$+{numpart}$+{suf}";
	print "replacing $n1 with $n2\n" unless $QUIET;
	$delta_worked |= delta_test ();
    }     
    $first = substr($prog, 0, $delta_pos);
    $rest = substr($prog, $delta_pos);
    $orig_rest = $rest;
    if ($rest =~ s/^(?<pref>$borderorspc(\\-|\\+)?(0|(0[xX]))?)(?<numpart>[0-9a-fA-F]+)(?<suf1>[ULul]*)(?<suf2>$borderorspc)/$+{pref}$+{numpart}$+{suf2}/sm && ($rest ne $orig_rest)) {
	$prog = $first.$rest;
	my $n1 = "$+{pref}$+{numpart}$+{suf1}$+{suf2}";
		my $n2 = "$+{pref}$+{numpart}$+{suf2}";
	print "replacing $n1 with $n2\n" unless $QUIET;
	$delta_worked |= delta_test ();
    } 
}

sub angles () {
    if (substr($prog, $delta_pos, 1) eq "<") {
	my $p2 = find_match ($delta_pos+1,"<",">");
	if ($p2 != -1) {
	    die if (substr($prog, $delta_pos, 1) ne "<");
	    die if (substr($prog, $p2, 1) ne ">");
	    
	    my $del = substr ($prog, $delta_pos, $p2-$delta_pos+1, "");
	    print "deleting '$del' at $delta_pos--$p2 : " unless $QUIET;
	    my $res = delta_test ();
	    $delta_worked |= $res;
	    
	    if (!$res) {
		substr ($prog, $p2, 1) = "";
		substr ($prog, $delta_pos, 1) = "";
		print "deleting at $delta_pos--$p2 : " unless $QUIET;
		$delta_worked |= delta_test ();
	    }
	}
    }
}

sub parens () {
    if (substr($prog, $delta_pos, 1) eq "(") {
	my $p2 = find_match ($delta_pos+1,"(",")");
	if ($p2 != -1) {
	    die if (substr($prog, $delta_pos, 1) ne "(");
	    die if (substr($prog, $p2, 1) ne ")");
	    
	    my $del = substr ($prog, $delta_pos, $p2-$delta_pos+1, "");
	    print "deleting '$del' at $delta_pos--$p2 : " unless $QUIET;
	    my $res = delta_test ();
	    $delta_worked |= $res;
	    
	    if (!$res) {
		substr ($prog, $p2, 1) = "";
		substr ($prog, $delta_pos, 1) = "";
		print "deleting at $delta_pos--$p2 : " unless $QUIET;
		$delta_worked |= delta_test ();
	    }
	}
    }
}

sub brackets () {
    if (substr($prog, $delta_pos, 1) eq "{") {
	my $p2 = find_match ($delta_pos+1,"{","}");
	if ($p2 != -1) {
	    die if (substr($prog, $delta_pos, 1) ne "{");
	    die if (substr($prog, $p2, 1) ne "}");
	    
	    my $del = substr ($prog, $delta_pos, $p2-$delta_pos+1, "");
	    print "deleting '$del' at $delta_pos--$p2 : " unless $QUIET;
	    my $res = delta_test ();
	    $delta_worked |= $res;
	    
	    if (!$res) {
		substr ($prog, $p2, 1) = "";
		substr ($prog, $delta_pos, 1) = "";
		print "deleting at $delta_pos--$p2 : " unless $QUIET;
		$delta_worked |= delta_test ();
	    }
	}
    }
}

############################## end delta passes #############################

sub call_method ($) {
    no strict "refs";
    ($delta_method) = @_;    
    &$delta_method();
}

sub round ($) {
    (my $n) = @_;
    return int ($n+0.5);
}

sub delta_pass ($) {
    ($delta_method) = @_;    
    $delta_pos = 0;
    $good_cnt = 0;
    $bad_cnt = 0;

    print "\n" unless $QUIET;
    print "========== starting pass <$delta_method> ==========\n";

    my $chunk_size;
    if ($delta_method =~ /^lines([0-9]*)$/) {
	my $topform = $1;
	if (defined($1)) {
	    system "topformflat $1 < $cfile > tmpfile";
	    system "mv tmpfile $cfile";
	    $changed_on_disk = 1;
	    delta_test();
	}
	$chunk_size = count_lines();
    }

  again:

    if ($SANITY) {
	sanity_check();
    }

    $exit_delta_pass = 0;
    
    while (1) {
	ensure_mem_and_disk_are_synced();
	return if ($delta_pos >= length ($prog));
	$delta_worked = 0;

	if ($delta_method =~ /^clang-(.*)$/) {
	    my $clang_delta_method = $1;
	    clang_delta ($clang_delta_method);
	} elsif ($delta_method =~ /^lines/) {
	    lines ($chunk_size);
	} else {
	    call_method($delta_method);
	} 

	if ($exit_delta_pass) {
	    
	    if ($delta_method =~ /^lines/ && $chunk_size > 1) {
		$chunk_size = round ($chunk_size / 2.0);
		printf "new chunk size = $chunk_size\n" unless $QUIET;
		$delta_pos = 0;
		goto again;
	    }
	    return;
	}

	if (!$delta_worked) {
	    $delta_pos++;
	}
    }
}

############################### main #################################

# global invariant: the delta test always succeeds for $cfile.bak

my %all_methods = (

    "crc" => 1,
    "angles" => 2,
    "brackets" => 2,
    "ternary" => 2,
    "parens" => 3,
    "indent" => 3,
    "replace_regex" => 4,
    "shorten_ints" => 5,
    "blanks" => 14,

    );

my $clang_delta = File::Which::which ("clang_delta");
if (defined($clang_delta)) {
    $all_methods{"clang-aggregate-to-scalar"} = 10;
    # $all_methods{"clang-binop-simplification"} = 10;
    $all_methods{"clang-local-to-global"} = 10;
    $all_methods{"clang-param-to-global"} = 10;
    $all_methods{"clang-param-to-local"} = 10;
    $all_methods{"clang-remove-nested-function"} = 10;
    $all_methods{"clang-remove-unused-function"} = -10;
    $all_methods{"clang-rename-fun"} = 10;
    $all_methods{"clang-rename-param"} = 10;    
    $all_methods{"clang-rename-var"} = 10;
    $all_methods{"clang-replace-callexpr"} = 10;    
    $all_methods{"clang-return-void"} = 10;
    $all_methods{"clang-simple-inliner"} = 10;
    $all_methods{"clang-reduce-pointer-level"} = 10;
    $all_methods{"clang-lift-assignment-expr"} = 10;
    $all_methods{"clang-copy-propagation"} = 10;
    $all_methods{"clang-remove-unused-var"} = 10;
    $all_methods{"clang-simplify-callexpr"} = 10;
    $all_methods{"clang-callexpr-to-value"} = 10;
} else {
    printf ("clang_delta not found in path, disabling its passes\n");
}

my $topformflat = File::Which::which ("topformflat");
if (defined($topformflat)) {
    $all_methods{"lines0"} = 15;
    $all_methods{"lines1"} = 14;
    $all_methods{"lines2"} = 13;
    $all_methods{"lines10"} = 12;
} else {
    $all_methods{"lines"} = 15;
    printf ("topformflat not found in path, disabling its passes\n");
}

sub usage() {
    print "usage: c_delta.pl test_script.sh file.c [method [method ...]]\n";
    print "available methods are:\n";
    foreach my $method (keys %all_methods) {
	print "  --$method\n";
    }
    print "Default strategy is trying all methods.\n";
    die;
}

$test = shift @ARGV;
usage if (!defined($test));
if (!(-x $test)) {
    print "test script '$test' not found, or not executable\n";
    usage();
}

$cfile = shift @ARGV;
usage if (!defined($cfile));
if (!(-e $cfile)) {
    print "'$cfile' not found\n";
    usage();
}

my %methods = ();
foreach my $arg (@ARGV) {
    my $found = 0;
    foreach my $method (keys %all_methods) {
	if ($arg eq "--$method") {
	    $methods{$method} = 1;
	    $found = 1;
	    last;
	}
    }
    if (!$found) {
	print "unknown method '$arg'\n";
	usage();
    }
}

if (scalar (keys %methods) == 0) {
    # default to all methods
    foreach my $method (keys %all_methods) {
	$methods{$method} = 1;
    }
}

system "cp $cfile $cfile.orig";
system "cp $cfile $cfile.bak";

sub bymethod {
    return $all_methods{$a} <=> $all_methods{$b};
}

# iterate to global fixpoint

read_file ();    
$orig_prog_len = length ($prog);

my $file_size = -s $cfile;
my $spinning = 0;

if (1) {
    delta_pass ("lines0");
    delta_pass ("lines1");
    delta_pass ("lines2");
    delta_pass ("lines10");
    delta_pass ("clang-remove-unused-function");
    delta_pass ("clang-remove-unused-var");
    delta_pass ("clang-callexpr-to-value");
    delta_pass ("clang-simplify-callexpr");
    delta_pass ("clang-remove-unused-function");
}

while (1) {
    foreach my $method (sort bymethod keys %methods) {
	delta_pass ($method);
    }
    $pass_num++;
    my $s = -s $cfile;
    print "Termination check: size was $file_size; now $s\n";
    last if ($s >= $file_size);
    $file_size = $s;
}

if (1) {
    delta_pass ("clang-combine-global-var");
    delta_pass ("clang-combine-local-var");
    delta_pass ("final_indent");
}

print "===================== done ====================\n";

print "\n";
print "overall reduction: ";
print_pct();

print "\n";
print "pass statistics:\n";
foreach my $method (sort keys %methods) {
    my $w = $method_worked{$method};
    $w=0 unless defined($w);
    my $f = $method_failed{$method};
    $f=0 unless defined($f);
    print "  method $method worked $w times and failed $f times\n";
}

print "\n";
print "regex statistics:\n";
for (my $n=0; $n<scalar(@regexes_to_replace); $n++) {
    my $a = $regex_worked{$n};
    my $b = $regex_failed{$n};
    next if (($a+$b)==0);
    print "  $n s:$a f:$b\n";
}

print "\n";
print "delimited regex statistics:\n";
for (my $n=0; $n<scalar(@delimited_regexes_to_replace); $n++) {
    my $a = $delimited_regex_worked{$n};
    my $b = $delimited_regex_failed{$n};
    next if (($a+$b)==0);
    print "  $n s:$a f:$b\n";
}

print "\n";

print "reduced test case:\n\n";
system "cat $cfile";

######################################################################
