#!/usr/bin/perl -w

######################################################################

use strict;

######################################################################

my $DEBUG = 0;
my $SANITY = 1;

######################################################################

my $prog;
my $orig_prog_len;

sub print_pct () {
    my $pct = 100 - (length($prog)*100.0/$orig_prog_len);
    printf "(%.1f %%)\n", $pct;
}

# these are set at startup time and never change
my $cfile;
my $test;
my $trial_num = 0;   

sub read_file () {
    open INF, "<$cfile" or die;
    $prog = "";
    while (my $line = <INF>) {
	$prog .= $line;
    }
    if (substr($prog, 0, 1) ne " ") {
	$prog = " $prog";
    }
    if (substr ($prog, -1, 1) ne " ") {
	$prog = "$prog ";
    }
    close INF;
}

sub save_copy ($) {
    (my $fn) = @_;
    open OUTF, ">$fn" or die;
    print OUTF $prog;
    close OUTF;
}

sub write_file () {
    if (defined($DEBUG) && $DEBUG) {
	save_copy ("delta_tmp_${trial_num}.c");
    }
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

my %cache = ();
my $cache_hits = 0;
my $good_cnt;
my $bad_cnt;
my $pass_num = 0;
my %method_worked = ();
my %method_failed = ();
my $old_size = 1000000000;
 
sub delta_test ($$$) {
    (my $method, my $number, my $total) = @_;
    my $len = length ($prog);
    print "[$pass_num $method ($number / $total) s:$good_cnt f:$bad_cnt] ";

    my $result = $cache{$prog};

    if (defined($result)) {
	$cache_hits++;
	print "(hit) ";
	print "failure\n";
	read_file ();    
	$bad_cnt++;
	$method_failed{$method}++;
	return 0;
    }
    
    write_file ();
    $result = run_test ();
    $cache{$prog} = $result;
    
    if ($result) {
	print "success ";
	print_pct();
	system "cp $cfile $cfile.bak";
	$good_cnt++;
	$method_worked{$method}++;
	my $size = length ($prog);
	if ($size < $old_size) {
	    %cache = ();
	}
	$old_size = $size;
	return 1;
    } else {
	print "failure\n";
	system "cp $cfile.bak $cfile";
	read_file ();    
	$bad_cnt++;
	$method_failed{$method}++;
	return 0;
    }
}

sub sanity_check () {
    print "sanity check... ";
    my $res = run_test ();
    if (!$res) {
	die "test (and sanity check) fails";
    }
    print "successful\n";
}

sub delta_pass ($) {
    (my $method) = @_;
    
    my $number = 0;
    $good_cnt = 0;
    $bad_cnt = 0;
    %funcs_seen = ();

    if ($SANITY) {
	sanity_check();
    }

    print "\n";
    print "========== starting pass <$method> ==========\n";

}

# invariant: test always succeeds for $cfile.bak

my %all_methods = (

    );
 
############################### main #################################

sub usage() {
    print "usage: reduce.pl test_script.sh testcase [method, method, ...]]\n";
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

#print "methods: ";
#foreach (sort keys %methods) {
#    print "$_ ";
#}
#print "\n";

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

while (1) {
    save_copy ("delta_backup_${pass_num}.c");
    my $success = 0;
    foreach my $method (sort bymethod keys %methods) {
	$success |= delta_pass ($method);
    }
    $pass_num++;
    last if (!$success);
    my $s = -s $cfile;
    if ($s >= $file_size) {
	$spinning++;
    }
    last if ($spinning > 3);
    $file_size = $s;
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
print "there were $cache_hits cache hits\n";

######################################################################
