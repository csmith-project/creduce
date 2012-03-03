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

# global invariant: the delta test always succeeds for $cfile.bak
sub delta_test ($$$) {
    (my $delta_pos, my $method, my $arg) = @_;
    my $prog = read_file($cfile);
    my $len = length ($prog);

    print "[$pass_num $method :: $arg ($delta_pos / $len) s:$good_cnt f:$bad_cnt] " 
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
    &${str}($fn,$pos);
}

sub call_method ($$$$) {
    my ($method,$fn,$pos,$arg) = @_;    
    my $str = $method."::transform";
    no strict "refs";
    &${str}($fn,$pos,$arg);
}

sub round ($) {
    (my $n) = @_;
    return int ($n+0.5);
}

sub delta_pass ($) {
    (my $mref) = @_;    
    my $delta_method = ${$mref}{"name"};
    my $delta_arg = ${$mref}{"arg"};
    my $delta_pos = 0;
    $good_cnt = 0;
    $bad_cnt = 0;

    print "\n" unless $QUIET;
    print "========== starting pass <$delta_method :: $delta_arg> ==========\n";

    my $chunk_size;
    if ($delta_method =~ /^lines([0-9]*)$/) {
	my $topform = $1;
	if (defined($1)) {
	    system "topformflat $1 < $cfile > tmpfile";
	    system "mv tmpfile $cfile";
	    delta_test($delta_pos, "topformflat", "");
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
	    $res = call_method($delta_method,$cfile,$delta_pos,$delta_arg);
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

	system "diff ${cfile}.bak $cfile";

	die unless ($res == $SUCCESS ||
		    $res == $FAILURE);

	if ($res == $FAILURE) {
	    $delta_pos++;
	} else {
	    $res = delta_test ($delta_pos, $delta_method, $delta_arg);
	    $delta_pos++ unless $res;
	}
    }
}

sub usage() {
    print "usage: c_reduce.pl test_script.sh file.c\n";
    die;
}

my @all_methods = ();

sub by_first_pass_priority {
    my $pa;
    my $pb;
    foreach my $mref (@all_methods) {
	my %m = %{$mref};
	if (${$a}{"name"} eq $m{"name"} &&
	    ${$a}{"arg"} eq $m{"arg"}) {
	    $pa = $m{"first_pass_priority"};
	}
	if (${$b}{"name"} eq $m{"name"} &&
	    ${$b}{"arg"} eq $m{"arg"}) {
	    $pb = $m{"first_pass_priority"};
	}
    }
    die unless defined ($pa);
    die unless defined ($pb);
    return $pa <=> $pb;
}

sub by_priority {
    my $pa;
    my $pb;

    # print ${$a}{"name"}." ".${$b}{"name"}."\n";

    foreach my $mref (@all_methods) {
	my %m = %{$mref};
	if (${$a}{"name"} eq $m{"name"} &&
	    ${$a}{"arg"} eq $m{"arg"}) {
	    $pa = $m{"priority"};
	}
	if (${$b}{"name"} eq $m{"name"} &&
	    ${$b}{"arg"} eq $m{"arg"}) {
	    $pb = $m{"priority"};
	}
    }
    die unless defined ($pa);
    die unless defined ($pb);
    return $pa <=> $pb;
}

sub by_last_pass_priority {
    my $pa;
    my $pb;
    foreach my $mref (@all_methods) {
	my %m = %{$mref};
	if (${$a}{"name"} eq $m{"name"} &&
	    ${$a}{"arg"} eq $m{"arg"}) {
	    $pa = $m{"last_pass_priority"};
	}
	if (${$b}{"name"} eq $m{"name"} &&
	    ${$b}{"arg"} eq $m{"arg"}) {
	    $pb = $m{"last_pass_priority"};
	}
    }
    die unless defined ($pa);
    die unless defined ($pb);
    return $pa <=> $pb;
}

sub has_priority { 
    return defined(${$_}{"priority"});
}

sub has_first_pass_priority { 
    return defined(${$_}{"first_pass_priority"});
}

sub has_last_pass_priority { 
    return defined(${$_}{"last_pass_priority"});
}

############################### main #################################

# put this into a config file?
@all_methods = (
    { 
	"name" => "pass_ternary",
	"arg" => "b",
	"priority" => 5,
    },
    { 
	"name" => "pass_ternary",
	"arg" => "c",
	"priority" => 5,
    },
    { 
	"name" => "pass_balanced",
	"arg" => "curly",
	"priority" => 10,
    },
    { 
	"name" => "pass_balanced",
	"arg" => "parens",
	"priority" => 11,
    },
    { 
	"name" => "pass_balanced",
	"arg" => "angles",
	"priority" => 12,
    },
    { 
	"name" => "pass_balanced",
	"arg" => "curly-only",
	"priority" => 50,
    },
    { 
	"name" => "pass_balanced",
	"arg" => "parens-only",
	"priority" => 51,
    },
    { 
	"name" => "pass_balanced",
	"arg" => "angles-only",
	"priority" => 52,
    },
    {
	"name" => "pass_indent",
	"arg" => "",
	"priority" => 100,
    },
    #{
    #	"name" => "pass_blank",
    #	"priority" => 101,
    #},
    
    {
	"name" => "pass_indent_final",
	"arg" => "",
	"last_pass_priority" => 100,
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
    if (!(&${str}())) {
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

# some stuff we run first since it often makes good headway quickly
print "INITIAL PASS\n";
foreach my $method (sort by_first_pass_priority grep (has_first_pass_priority, @all_methods)) {
    delta_pass ($method);
}

# iterate to global fixpoint
print "MAIN PASSES\n";
$file_size = -s $cfile;
while (1) {
    foreach my $method (sort by_priority grep (has_priority, @all_methods)) {
	delta_pass ($method);
    }
    $pass_num++;
    my $s = -s $cfile;
    print "Termination check: size was $file_size; now $s\n";
    last if ($s >= $file_size);
    $file_size = $s;
}

# some stuff we run last since it only makes sense as cleanup
print "CLEANUP PASS\n";
foreach my $method (sort by_last_pass_priority grep (has_last_pass_priority, @all_methods)) {
    delta_pass ($method);
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
