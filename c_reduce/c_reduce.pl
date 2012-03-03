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

# global invariant: the delta test always succeeds for $cfile.bak

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

sub count_lines () {
    open INF, "<$cfile" or die;
    my $n=0;
    $n++ while (<INF>);
    close INF;
    return $n;
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

sub init_method ($) {
    (my $method,my $fn,my $pos) = @_;    
    my $str = $method."::init";
    no strict "refs";
    &$str($fn,$pos);
}

sub call_method ($$$$) {
    my ($method,$fn,$pos,$arg) = @_;    
    my $str = $method."::transform";
    no strict "refs";
    &$str($fn,$pos,$arg);
}

sub round ($) {
    (my $n) = @_;
    return int ($n+0.5);
}

sub delta_pass ($) {
    (my $mref) = @_;    
    my $delta_method = ${$mref}{"name"};
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

    init_method ($delta_method);

    while (1) {

	my $res;

	if ($delta_method =~ /^clang-(.*)$/) {
	    my $clang_delta_method = $1;
	    $res = clang_delta ($clang_delta_method);
	} elsif ($delta_method =~ /^lines/) {
	    $res = lines ($chunk_size);
	} else {
	    my $arg = ${$mref}{"arg"};
	    $res = call_method($delta_method,$cfile,$delta_pos,$arg);
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

my %all_methods = ();

sub usage() {
    print "usage: c_reduce.pl test_script.sh file.c [method [method ...]]\n";
    print "available methods are:\n";
    foreach my $method (keys %all_methods) {
	print "  --$method\n";
    }
    print "Default strategy is trying all methods.\n";
    die;
}

sub bymethod {
    return $all_methods{$a} <=> $all_methods{$b};
}

############################### main #################################

# put this into a config file?
my @all_methods = (
    { 
	"name" => "pass_ternary",
	"arg" => "b",
	"priority" => "5",
    },
    { 
	"name" => "pass_ternary",
	"arg" => "c",
	"priority" => "5",
    },
    {
	"name" => "pass_indent",
	"priority" => 1,
    },
    {
	"name" => "pass_indent_final",
	"priority" => 100,
    },
    );

my %prereqs_checked;
foreach my $mref (@all_methods) {
    my %method = %{$mref};
    my $mname = $method{"name"};
    die unless defined ($mname);
    next if defined ($prereqs_checked{$mname});
    $prereqs_checked{$mname} = 1;
    eval "require $mname";
    my $str = $mname."::check_prereqs";
    no strict "refs";
    if (!(&$str())) {
	die "prereqs not found for pass $mname";
    }
    print "successfully checked prereqs for $mname\n" unless $QUIET;
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

# iterate to global fixpoint

sub bypriority {
    my $pa;
    my $pb;
    foreach my $mref (@all_methods) {
	my %m = %{$mref};
	if (${$a}{"name"} eq $m{"name"}) {
	    $pa = $m{"priority"};
	}
	if (${$b}{"name"} eq $m{"name"}) {
	    $pb = $m{"priority"};
	}
    }
    die unless defined ($pa);
    die unless defined ($pb);
    return $pa <=> $pb;
}

while (1) {
    foreach my $method (sort bypriority @all_methods) {
	delta_pass ($method);
    }
    $pass_num++;
    my $s = -s $cfile;
    print "Termination check: size was $file_size; now $s\n";
    last if ($s >= $file_size);
    $file_size = $s;
}

print "===================== done ====================\n";

print "\n";
print "pass statistics:\n";
foreach my $mref (sort @all_methods) {
    my $method = ${$mref}{"name"};
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
