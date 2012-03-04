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
use FindBin;
use lib $FindBin::Bin;

use creduce_utils;

######################################################################

# if set, ensure the delta test succeeds before starting each pass
my $SANITY = 1;

# if set, don't print a lot of stuff
# (there's no "real quiet" option -- just redirect output to /dev/null
# if you don't want to see it)
my $QUIET = 0;

######################################################################

my $orig_file_size;

sub print_pct ($) {
    (my $l) = @_;
    my $pct = 100 - ($l*100.0/$orig_file_size);
    printf "(%.1f %%)\n", $pct;
}

# these are set at startup time and never change
my $cfile;
my $test;
my $trial_num = 0;   

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

# global invariant: the delta test always succeeds for $cfile.bak
sub delta_test ($$) {
    (my $method, my $arg) = @_;
    my $prog = read_file($cfile);
    my $len = length ($prog);

    print "[$pass_num $method :: $arg s:$good_cnt f:$bad_cnt] " 
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
	print_pct(-s $cfile);
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

sub call_prereq_check ($) {
    (my $method) = @_;
    my $str = $method."::check_prereqs";
    no strict "refs";
    if (!(&${str}())) {
	die "prereqs not found for pass $method";
    }
    print "successfully checked prereqs for $method\n" unless $QUIET;
}

sub call_init ($) {
    (my $method) = @_;    
    my $str = $method."::init";
    no strict "refs";
    &${str}();
}

sub call_advance ($) {
    (my $method) = @_;    
    my $str = $method."::advance";
    no strict "refs";
    &${str}();
}

sub call_method ($$$) {
    my ($method,$fn,$arg) = @_;    
    my $str = $method."::transform";
    no strict "refs";
    &${str}($fn,$arg);
}

sub delta_pass ($) {
    (my $mref) = @_;    
    my $delta_method = ${$mref}{"name"};
    my $delta_arg = ${$mref}{"arg"};
    $good_cnt = 0;
    $bad_cnt = 0;

    print "\n" unless $QUIET;
    print "========== starting pass <$delta_method :: $delta_arg> ==========\n";

    if ($SANITY) {
	sanity_check();
    }

    call_init ($delta_method);

    while (1) {

	my $res;
	$res = call_method($delta_method,$cfile,$delta_arg);

	if ($res == $STOP) {
	    return;
	}
	
	system "diff ${cfile}.bak $cfile";

	die unless ($res == $SUCCESS ||
		    $res == $FAILURE);

	if ($res == $SUCCESS) {
	    $res = delta_test ($delta_method, $delta_arg);
	    call_advance($delta_method) unless $res;
	} else {
	    call_advance($delta_method);
	}
    }
}

sub usage() {
    print "usage: c_reduce.pl test_script.sh file.c\n";
    die;
}

my @all_methods = ();

sub by_first_pass_pri {
    my $pa;
    my $pb;
    foreach my $mref (@all_methods) {
	my %m = %{$mref};
	if (${$a}{"name"} eq $m{"name"} &&
	    ${$a}{"arg"} eq $m{"arg"}) {
	    $pa = $m{"first_pass_pri"};
	}
	if (${$b}{"name"} eq $m{"name"} &&
	    ${$b}{"arg"} eq $m{"arg"}) {
	    $pb = $m{"first_pass_pri"};
	}
    }
    die unless defined ($pa);
    die unless defined ($pb);
    return $pa <=> $pb;
}

sub by_pri {
    my $pa;
    my $pb;

    # print ${$a}{"name"}." ".${$b}{"name"}."\n";

    foreach my $mref (@all_methods) {
	my %m = %{$mref};
	if (${$a}{"name"} eq $m{"name"} &&
	    ${$a}{"arg"} eq $m{"arg"}) {
	    $pa = $m{"pri"};
	}
	if (${$b}{"name"} eq $m{"name"} &&
	    ${$b}{"arg"} eq $m{"arg"}) {
	    $pb = $m{"pri"};
	}
    }
    die unless defined ($pa);
    die unless defined ($pb);
    return $pa <=> $pb;
}

sub by_last_pass_pri {
    my $pa;
    my $pb;
    foreach my $mref (@all_methods) {
	my %m = %{$mref};
	if (${$a}{"name"} eq $m{"name"} &&
	    ${$a}{"arg"} eq $m{"arg"}) {
	    $pa = $m{"last_pass_pri"};
	}
	if (${$b}{"name"} eq $m{"name"} &&
	    ${$b}{"arg"} eq $m{"arg"}) {
	    $pb = $m{"last_pass_pri"};
	}
    }
    die unless defined ($pa);
    die unless defined ($pb);
    return $pa <=> $pb;
}

sub has_pri { 
    return defined(${$_}{"pri"});
}

sub has_first_pass_pri { 
    return defined(${$_}{"first_pass_pri"});
}

sub has_last_pass_pri { 
    return defined(${$_}{"last_pass_pri"});
}

@all_methods = (
    { "name" => "pass_ternary",  "arg" => "b",                      "pri" => 105,  },
    { "name" => "pass_ternary",  "arg" => "c",                      "pri" => 105,  },
    { "name" => "pass_balanced", "arg" => "curly",                  "pri" => 110,  },
    { "name" => "pass_balanced", "arg" => "parens",                 "pri" => 111,  },
    { "name" => "pass_balanced", "arg" => "angles",                 "pri" => 112,  },
    { "name" => "pass_balanced", "arg" => "curly-only",             "pri" => 150,  },
    { "name" => "pass_balanced", "arg" => "parens-only",            "pri" => 151,  },
    { "name" => "pass_balanced", "arg" => "angles-only",            "pri" => 152,  },
    { "name" => "pass_clang",    "arg" => "aggregate-to-scalar",    "pri" => 200,  },
   #{ "name" => "pass_clang",    "arg" => "binop-simplification",   "pri" => 201,  },
    { "name" => "pass_clang",    "arg" => "local-to-global",        "pri" => 202,  },
    { "name" => "pass_clang",    "arg" => "param-to-global",        "pri" => 203,  },
    { "name" => "pass_clang",    "arg" => "param-to-local",         "pri" => 204,  },
    { "name" => "pass_clang",    "arg" => "remove-nested-function", "pri" => 205,  },
    { "name" => "pass_clang",    "arg" => "remove-unused-function", "pri" => 206,  "first_pass_pri" => 100, },
    { "name" => "pass_clang",    "arg" => "rename-fun",             "pri" => 207,  },
    { "name" => "pass_clang",    "arg" => "union-to-struct",        "pri" => 208,  },
    { "name" => "pass_clang",    "arg" => "rename-param",           "pri" => 209,  },
    { "name" => "pass_clang",    "arg" => "rename-var",             "pri" => 210,  },
    { "name" => "pass_clang",    "arg" => "replace-callexpr",       "pri" => 211,  },
    { "name" => "pass_clang",    "arg" => "return-void",            "pri" => 212,  },
    { "name" => "pass_clang",    "arg" => "simple-inliner",         "pri" => 213,  },
    { "name" => "pass_clang",    "arg" => "reduce-pointer-level",   "pri" => 214,  },
    { "name" => "pass_clang",    "arg" => "lift-assignment-expr",   "pri" => 215,  },
    { "name" => "pass_clang",    "arg" => "copy-propagation",       "pri" => 216,  },
    { "name" => "pass_clang",    "arg" => "remove-unused-var",      "pri" => 217,  "first_pass_pri" => 101, },
    { "name" => "pass_clang",    "arg" => "simplify-callexpr",      "pri" => 218,  "first_pass_pri" => 104, },
    { "name" => "pass_clang",    "arg" => "callexpr-to-value",      "pri" => 219,  "first_pass_pri" => 102, },
    { "name" => "pass_clang",    "arg" => "union-to-struct",        "pri" => 220,  },
    { "name" => "pass_clang",    "arg" => "simplify-if",            "pri" => 221,  },
    { "name" => "pass_clang",    "arg" => "combine-global-var",                    "last_pass_pri" => 990, },
    { "name" => "pass_clang",    "arg" => "combine-local-var",                     "last_pass_pri" => 991, },
    { "name" => "pass_ints",     "arg" => "a",                      "pri" => 400,  },
    { "name" => "pass_ints",     "arg" => "b",                      "pri" => 401,  },
    { "name" => "pass_ints",     "arg" => "c",                      "pri" => 402,  },
    { "name" => "pass_ints",     "arg" => "d",                      "pri" => 403,  },
    { "name" => "pass_ints",     "arg" => "e",                      "pri" => 403,  },
    { "name" => "pass_lines",    "arg" => "0",                      "pri" => 410,  "first_pass_pri" => 10, },
    { "name" => "pass_lines",    "arg" => "1",                      "pri" => 411,  "first_pass_pri" => 11, },
    { "name" => "pass_lines",    "arg" => "2",                      "pri" => 412,  "first_pass_pri" => 12, },
    { "name" => "pass_lines",    "arg" => "10",                     "pri" => 413,  "first_pass_pri" => 13, },
    { "name" => "pass_indent",   "arg" => "regular",                "pri" => 1000, },
    { "name" => "pass_indent",   "arg" => "final",                                 "last_pass_pri" => 1000, },
    );

############################### main #################################

my %prereqs_checked;
foreach my $mref (@all_methods) {
    my %method = %{$mref};
    my $mname = $method{"name"};
    die unless defined ($mname);
    next if defined ($prereqs_checked{$mname});
    $prereqs_checked{$mname} = 1;
    eval "require $mname";
    call_prereq_check($mname);
}
print "\n" unless $QUIET;

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

system "cp $cfile $cfile.orig";
system "cp $cfile $cfile.bak";

my $file_size = -s $cfile;
$orig_file_size = $file_size;

# some passes we run first since they often make good headway quickly
print "INITIAL PASS\n" unless $QUIET;
foreach my $method (sort by_first_pass_pri grep (has_first_pass_pri, @all_methods)) {
    delta_pass ($method);
}

# iterate to global fixpoint
print "MAIN PASSES\n" unless $QUIET;
$file_size = -s $cfile;
while (1) {
    foreach my $method (sort by_pri grep (has_pri, @all_methods)) {
	delta_pass ($method);
    }
    $pass_num++;
    my $s = -s $cfile;
    print "Termination check: size was $file_size; now $s\n";
    last if ($s >= $file_size);
    $file_size = $s;
}

# some passes we run last since they work best as cleanup
print "CLEANUP PASS\n" unless $QUIET;
foreach my $method (sort by_last_pass_pri grep (has_last_pass_pri, @all_methods)) {
    delta_pass ($method);
}

print "===================== done ====================\n";

print "\n";
print "pass statistics:\n";
foreach my $mref (sort @all_methods) {
    my $method = ${$mref}{"name"};
    my $arg = ${$mref}{"arg"};
    my $w = $method_worked{$method};
    $w=0 unless defined($w);
    my $f = $method_failed{$method};
    $f=0 unless defined($f);
    print "  method $method :: $arg worked $w times and failed $f times\n";
}

print "\n";

#print "reduced test case:\n\n";
#system "cat $cfile";

######################################################################
