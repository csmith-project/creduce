## -*- mode: Perl -*-
##
## Copyright (c) 2016 The University of Utah
## Copyright (c) 2016 Ori Brostovski <ori@ceemple.com>, <oribrost@gmail.com>
## Copyright (c) 2016 Ceemple Software Ltd
##
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################
# test run with:  creduce --no-cache --n 1 --no-default-passes --add-pass pass_defines 0 0 `which true` bubu

package pass_defines;

use strict;
use warnings;

use File::Compare;
use creduce_utils;

my $clang;

sub backslash_x ($) { 
    return sprintf("\\x%X", ord($_[0])); 
}

sub check_prereqs () {
    $clang =
        find_external_program(creduce_config::CLANG, "clang");
    return defined ($clang);
}

sub new ($$) {
    my $flag = 0;
    return \$flag;
}

sub advance ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my $flag = ${$state};
    $flag = 1;
    return \$flag;
}

sub advance_on_success ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    return $state;    
}
sub addI ($) { 
    return "-I" . $_[0]; 
};

sub do_transform ($) {
    (my $cfile) = @_;
    my $suffix = substr($cfile,rindex($cfile,"."));

    open INF, "<$cfile" or die;
    my $tmpfile1 = File::Temp::tmpnam() . $suffix;
    open TF1, ">$tmpfile1" or die;
    my $tmpfile2 = File::Temp::tmpnam() . $suffix;
    my $tmpfile3 = File::Temp::tmpnam() . $suffix;
    my $index = 0;
    
    while (my $line = <INF>) {
        if ($line =~ m/^\s*#\s*include/) {
            my $sline = $line;
            $sline =~ s/([^a-zA-Z0-9])/backslash_x($1)/ge;
            print TF1 "char *__creduce_before_include_" . $index . " = \"$sline\";\n";
            print TF1 $line;
            print TF1 "char *__creduce_after_include_" . $index . " = \"$sline\";\n";
        } else {
            print TF1 $line;
        }
    }
    close TF1;
    close INF;
    
    my $iargs = "";
    if (exists $ENV{CREDUCE_INCLUDE_PATH}) {
        my @args = split(/:/,$ENV{CREDUCE_INCLUDE_PATH});
        $iargs = "";
        foreach my $iarg (@args) {
            $iargs = $iargs . " -I" . $iarg;
        }
    }
    my $lang_arg = "";
    if (exists $ENV{CREDUCE_LANG}) {
        if ($ENV{CREDUCE_LANG} eq "CXX") {
            $lang_arg = "-x c++";
        } elsif ($ENV{CREDUCE_LANG} eq "C") {
            $lang_arg = "-x c"
        }
    }
    system("$clang -Wno-pragma-system-header-outside-header -E $lang_arg " .
           "$iargs $tmpfile1 -o $tmpfile2");

    unless (open TF2, "<$tmpfile2") {
        return 0;
    }
    open OUTF, ">$tmpfile3" or die;
    my $in_include = 0;
    while (my $line = <TF2>) {
        if ($in_include) {
            if ($line =~ m/^char \*__creduce_after_include/) {
                $in_include = 0;
            }
        } else {
            if ($line =~ m/^char \*__creduce_before_include.*= \"(.*)\"/) {
                my $value = $1;
                $value =~ s/\\xA$/\n/;
                $value =~ s/\\x(..)/chr(hex($1))/ge;
                print OUTF $value;
                $in_include = 1;
            } elsif ($line !~ /^# /) {
                print OUTF $line;
            }
        }
    }
    close OUTF;
    close TF2;

    unlink $tmpfile1;
    unlink $tmpfile2;

    if (compare($tmpfile3, $cfile)) {
        File::Copy::move($tmpfile3, $cfile);
        return 1;
    } else {
        unlink $tmpfile3;
        return 0;
    }
}

sub transform($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my $flag = ${$state};

    if ($flag) {
        return ($STOP, \$flag);
    }
    
    my $success = do_transform($cfile);
    return ($success ? $OK : $STOP, \$flag);
}

1;
