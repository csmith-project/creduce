#!/usr/bin/perl

use strict;
use warnings;

my $keep_temp = 0;
my $invoke_compiler = 0;
my $verbose = 0;
my $SRC_FILE = "";
my @transformations = ();
my %transformation_results = ();
my %verified_results = ();

my $CLANG_DELTA = "./clang_delta";
my $WORKING_DIR = "./working_dir";

my $COMPILER = "gcc";

sub print_msg($) {
  my ($msg) = @_;

  print "$msg" if ($verbose);
}

sub runit($) {
    my ($cmd) = @_;
    print_msg("run: $cmd\n");
    my $res = system "$cmd";
    return (($? >> 8) || $res);
}

sub die_on_fail($) {
    my ($cmd) = @_;

    my $res = runit($cmd);
    die "Failed to execute: $cmd!\n" if ($res);
}

sub get_instance_num($) {
    my ($trans) = @_;

    my $clang_delta_cmd = "$CLANG_DELTA --query-instances=$trans $SRC_FILE";

    print_msg("Query the number of available instances for $trans\n");
    print_msg("$clang_delta_cmd\n");
    my @out = `$clang_delta_cmd`;
    die "Cannot query the number of instances for $trans!" 
        if ($? >> 8);

    my $num;
    if ($out[0] =~ m/Available transformation instances:[\s\t]*([0-9]+)/) {
       $num = $1; 
    }
    else {
        die "Bad output from clang_delta: $out[0]!";
    }
    
    print("Available instances[$num]\n");
    return $num;
}

sub do_one_transformation($$) {
    my ($trans, $counter) = @_;

    my $output = "$WORKING_DIR/$trans/$trans" . "_" . "$counter.c";

    my $clang_delta_cmd = "$CLANG_DELTA --transformation=$trans --counter=$counter --output=$output $SRC_FILE";
    print_msg("$clang_delta_cmd\n");

    print("  increasing counter[$counter] ...");
    my $res = runit($clang_delta_cmd);
    if ($res) {
        print("[FAIL]\n");
    }
    else {
        print("[SUCCESS]\n");
    }

    return $res;
}

sub verify_one_output($$) {
    my ($trans, $counter) = @_;

    my $cfile = "$WORKING_DIR/$trans/$trans" . "_" . "$counter.c";
    my $ofile = "$WORKING_DIR/$trans/$trans" . "_" . "$counter.o";
    my $compiler_cmd = "$COMPILER -c $cfile -o $ofile";
    print_msg("Invoking $COMPILER on $cfile ...\n");
    print_msg("$compiler_cmd\n");

    print "Compiling $cfile ...\n";
    my $res = runit($compiler_cmd);

    if ($res) {
        print("[FAIL]\n");
    }
    else {
        print("[SUCCESS]\n");
    }

    return $res;
}

sub do_one_test($) {
    my ($trans) = @_;

    if (!(grep { $trans eq $_ } @transformations)) {
      die "Unknown transformation: $trans!";
    }

    print "\nTesting $trans ...\n";
    my $test_dir = "$WORKING_DIR/$trans";
    print_msg("Making dir $test_dir ...\n");
    mkdir $test_dir or die;

    my $instance_num = get_instance_num($trans);

    my @results = ();
    my @verified_results = ();

    print("Running transformation[$trans] ...\n");
    for(my $i = 1; $i <= $instance_num; $i++) {
        my $orig_backup = "$WORKING_DIR/$trans/$trans" . "_0.c";
  
        print_msg("Copying original file...\n");
        die_on_fail("cp $SRC_FILE $orig_backup");

        my $result = do_one_transformation($trans, $i);
        push @results, $result;

        if ($invoke_compiler) {
            if (!$result) {
                my $verified_result = verify_one_output($trans, $i);
                push @verified_results, $verified_result;
            }
            else {
                push @verified_results, 1;
            }
        }
    }
    $transformation_results{$trans} = \@results;
    $verified_results{$trans} = \@verified_results;
}

sub doit() {
    foreach my $trans (@transformations) {
        do_one_test($trans);
    }
}

sub dump_one_results($) {
    my ($results_hash) = @_;

    foreach my $trans (keys %$results_hash) {
        my $trans_failed = 0;
        my @failed_counters = ();

        my $results = $results_hash->{$trans};
        for(my $i = 0; $i < scalar(@$results); $i++) {
            my $result = @$results[$i];
            if ($result) {
                $trans_failed++;
                push @failed_counters, ($i+1);
            }
        }
        print "  transformation[$trans]: ";
        if (!$trans_failed) {
            print "All instances suceeded!\n";
        }
        else {
            print "$trans_failed instances failed [" . join(",", @failed_counters) . "]\n";
        }
    }
}

sub dump_results() {
    print("\nTest Results:\n");

    print("\nTransformation results:\n");
    dump_one_results(\%transformation_results);
    
    return if (!$invoke_compiler);

    print("\nCompilation results:\n");
    dump_one_results(\%verified_results);
    print("\n");
}

sub finish() {
    return if ($keep_temp);

    print_msg("deleting $WORKING_DIR\n");
    system "rm -rf $WORKING_DIR\n";
}

sub prepare() {
    print_msg("Preparing testing dir ...\n");
    print_msg("rm -rf $WORKING_DIR\n");
    system "rm -rf $WORKING_DIR";

    print_msg("Creating $WORKING_DIR ...\n");
    mkdir $WORKING_DIR or die;

    print_msg("Querying available transformations ...\n");
    my @outs = `$CLANG_DELTA --transformations`;
    
    die "Failed to get transformations!" if (@outs < 1);
  
    print("There are " . scalar(@outs) . " transformations available:\n");
    foreach (@outs) {
        print("  $_");
        chomp $_;
        push @transformations, $_;
    }

    print "\nStart testing ...\n";
}

my $help_msg = 'This script is for testing clang_delta.
If -transformation=<name> option is not provided(see below), the script does the following work:
  (1) collect all available transformations
  (2) for each transformation, get the number of transformation instances for each transformation
  (3) run clang_delta with counter values from 1 to the number of the instances
  (4) [optional] if -verify-output is given, invoke gcc to compile the transformed output for each output
  (5) collect and dump test statistics

If -transformation=<name> is given, the script will only test the designated transformation through step (2) to (5)

Options:

test_transformation.pl [-transformation=<name>] [-keep] [-verify-output] [-verbose] source.c
  -transformation=<name>: specify a transformation to test [By default, the script will test all transformations]
  -keep: keep all intermediate transformed results
  -verify-output: invoke gcc on each transformed output
  -verbose: output detailed testing process
  source.c: file under test

';

sub print_help() {
  print $help_msg;  
}

sub main() {
    my $opt;
    my @unused = ();
    my $transformation;
    while(defined ($opt = shift @ARGV)) {
        if ($opt =~ m/^-(.+)=(.+)$/) {
            if ($1 eq "transformation") {
                $transformation = $2;
            }
            else {
                die "unknown option: $opt";
            }
        }
        elsif ($opt =~ m/^-(.+)$/) {
            if ($1 eq "keep") {
                $keep_temp = 1;
            }
            elsif ($1 eq "verify-output") {
                $invoke_compiler = 1;
            }
            elsif ($1 eq "verbose") {
                $verbose = 1;
            }
            elsif ($1 eq "help") {
                print_help();
                return;
            }
            else {
                print "Invalid options: $opt\n";
                print_help();
                die;
            }
        }
        else {
            push @unused, $opt;
        }
    }

    if (@unused == 1) {
        $SRC_FILE = $unused[0];
    }
    else {
        die "Please give a test file!";
    }

    prepare();
    if (defined($transformation)) {
        do_one_test($transformation);
    }
    else {
        doit();
    }
    dump_results();
    finish();
}

main();
