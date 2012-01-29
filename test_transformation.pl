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
my $CSMITH_BIN = "";

my $CSMITH_HOME = $ENV{"CSMITH_HOME"};
my $test_iterations = 100;
my $with_csmith = 0;

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

sub prepare_source_file($) {
    my ($test_path) = @_;

    my $srcfile = "$test_path/csmith_orig_file.c";
    system "rm -rf $srcfile";

    my $csmith_cmd = "$CSMITH_HOME/src/csmith --output $srcfile";
    print_msg("Generating C file...\n");
    print_msg("$csmith_cmd\n");
    die_on_fail($csmith_cmd);

    my $processed_file = "$test_path/preprocessed.c";
    system "rm -rf $processed_file";

    my $include_path = "$CSMITH_HOME/runtime";
    my $preprocessor = "$COMPILER -I$include_path -E $srcfile > $processed_file";
    print_msg("Preprocessing the generated C file..\n");
    print_msg("$preprocessor\n");
    die_on_fail($preprocessor);
    $SRC_FILE = $processed_file;
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
    my @veri_results = ();

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
                push @veri_results, $verified_result;
            }
            else {
                push @veri_results, 1;
            }
        }
    }
    $transformation_results{$trans} = \@results;
    $verified_results{$trans} = \@veri_results;
}

sub doit() {
    foreach my $trans (@transformations) {
        do_one_test($trans);
    }
}

sub dump_one_results($) {
    my ($results_hash) = @_;

    my $failure_flag = 0;

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
            $failure_flag = -1;
            print "$trans_failed instances failed [" . join(",", @failed_counters) . "]\n";
        }
    }
    return $failure_flag;
}

sub dump_results() {
    my $failure_flag;
    print("\nTest Results:\n");

    print("\nTransformation results:\n");
    $failure_flag = dump_one_results(\%transformation_results);
    
    return $failure_flag if (!$invoke_compiler || ($failure_flag == -1));

    print("\nCompilation results:\n");
    $failure_flag = dump_one_results(\%verified_results);
    print("\n");
    return $failure_flag;
}

sub finish() {
    return if ($keep_temp);

    print_msg("deleting $WORKING_DIR\n");
    system "rm -rf $WORKING_DIR\n";
}

sub do_tests_with_csmith($) {
    my ($trans) = @_;

    for (my $i = 0; $i < $test_iterations; $i++) {
        print "Test iteration [$i]...\n";
        prepare_source_file($WORKING_DIR);
        %transformation_results = ();
        %verified_results = ();
      
        if (defined($trans)) {
            do_one_test($trans);
        }
        else {
            doit();
        }

        my $failure = dump_results();
        if ($failure == -1) {
            return;
        }

        system "rm -rf $WORKING_DIR/*";
    }
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
  -with-csmith: invoke Csmith to generate a testing file. Please set CSMITH_HOME to make Csmith work correctly 
                (we are not allowed to pass source.c if this option is given)
  -iterations: testing iterations with Csmith
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
            elsif ($1 eq "iterations") {
                $test_iterations = $2;
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
            elsif ($1 eq "with-csmith") {
                $with_csmith = 1;
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
        die "Cannot have both -with-csmith and testing file!" if ($with_csmith);
        $SRC_FILE = $unused[0];
    }
    elsif ($with_csmith) {
        die "Please set CSMITH_HOME env!" if (!defined($CSMITH_HOME));
        prepare();
        do_tests_with_csmith($transformation);
        return;
        # Nothing to do
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
