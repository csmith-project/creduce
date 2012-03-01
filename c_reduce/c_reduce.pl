#!/usr/bin/perl -w

######################################################################
#
# This Delta debugger specifically targets C/C++ code. Its design
# point is to be slow, thorough, and stupid. We count on the delta
# test script being able to rapidly discard syntactically invalid
# delta variants (for example, by compiling them).
#
####################################################################

use strict;
use File::Which;

######################################################################

use FindBin;
use lib $FindBin::Bin;

use creduce_regexes;
use creduce_utils;

# pass modules

#use creduce_indent;
use creduce_ternary;

######################################################################

# if set, save all delta variants
my $DEBUG = 0;

# if set, ensure the delta test succeeds before starting each pass
my $SANITY = 1;

my $QUIET = 0;

######################################################################

my $orig_prog_len;

sub print_pct ($) {
    (my $l) = @_;
    my $pct = 100 - ($l*100.0/$orig_prog_len);
    printf "(%.1f %%)\n", $pct;
}

# these are set at startup time and never change
my $cfile;
my $test;
my $trial_num = 0;   

sub count_lines () {
    open INF, "<$cfile" or die;
    my $n=0;
    $n++ while (<INF>);
    close INF;
    return $n;
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

my %cache = ();
my $cache_hits = 0;

sub delta_test ($$) {
    (my $delta_pos, my $method) = @_;
    my $prog = read_file($cfile);
    my $len = length ($prog);

    print "[$pass_num $method ($delta_pos / $len) s:$good_cnt f:$bad_cnt] " 
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
	print_pct($len);
	system "cp $cfile $cfile.bak";
	$good_cnt++;
	$method_worked{$method}++;
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
	print "failure\n" unless $QUIET;
	system "cp $cfile.bak $cfile";
	$bad_cnt++;
	$method_failed{$method}++;
	return 0;
    }
}

sub call_method ($$$) {
    no strict "refs";
    (my $method,my $fn,my $pos) = @_;    
    &$method($fn,$pos);
}

sub round ($) {
    (my $n) = @_;
    return int ($n+0.5);
}

sub delta_pass ($) {
    (my $delta_method) = @_;    
    my $delta_pos = 0;
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
	    delta_test($delta_pos, "topformflat");
	}
	$chunk_size = count_lines();
    }

  again:

    if ($SANITY) {
	sanity_check();
    }

    while (1) {

	my $res;

	if ($delta_method =~ /^clang-(.*)$/) {
	    my $clang_delta_method = $1;
	    $res = clang_delta ($clang_delta_method);
	} elsif ($delta_method =~ /^lines/) {
	    $res = lines ($chunk_size);
	} else {
	    $res = call_method($delta_method,$cfile,$delta_pos);
	} 

	if ($res == $STOP) {
	    
	    if ($delta_method =~ /^lines/ && $chunk_size > 1) {
		$chunk_size = round ($chunk_size / 2.0);
		printf "new chunk size = $chunk_size\n" unless $QUIET;
		$delta_pos = 0;
		goto again;
	    }
	    return;
	}

	die unless ($res == $SUCCESS ||
		    $res == $FAILURE);

	if ($res == $FAILURE) {
	    $delta_pos++;
	} else {
	    $res = delta_test ($delta_pos, $delta_method);
	    $delta_pos++ unless $res;
	}
    }
}

############################### main #################################

# global invariant: the delta test always succeeds for $cfile.bak

my %all_methods = (

    "crc" => 1,
    "angles" => 2,
    "brackets" => 2,
    "ternary1" => 2,
    "ternary2" => 2,
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
    $all_methods{"clang-union-to-struct"} = 10;
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

###########FIXME
%all_methods = ();
$all_methods{"ternary1"} = 1;
$all_methods{"ternary2"} = 1;

sub usage() {
    print "usage: c_reduce.pl test_script.sh file.c [method [method ...]]\n";
    print "available methods are:\n";
    foreach my $method (keys %all_methods) {
	print "  --$method\n";
    }
    print "Default strategy is trying all methods.\n";
    die;
}

$test = shift @ARGV;
usage unless defined($test);
if (!(-x $test)) {
    print "test script '$test' not found, or not executable\n";
    usage();
}

$cfile = shift @ARGV;
usage unless defined($cfile);
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

{
    my $prog = read_file ($cfile);    
    $orig_prog_len = length ($prog);
}

my $file_size = -s $cfile;
my $spinning = 0;

if (0) {
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

if (0) {
    delta_pass ("clang-combine-global-var");
    delta_pass ("clang-combine-local-var");
    delta_pass ("final_indent");
}

print "===================== done ====================\n";

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

#print "reduced test case:\n\n";
#system "cat $cfile";

######################################################################
