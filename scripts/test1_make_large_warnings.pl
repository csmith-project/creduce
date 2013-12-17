#!/usr/bin/perl -w

use strict;

my $res;

my $DIR = "/home/regehr/z/tgceec/barehands/foo/reduce";

$res = system "g++ -std=c++11 -c foo.cpp > out.txt 2>&1";
exit -1 unless ($res==0);
my $s1 = -s "out.txt";
my $s2 = -s "$DIR/out_best.txt";
my $diff1 = $s1 - $s2;
print "improvement in warning size: $diff1\n";
my $p1 = -s "$DIR/foo.best";
my $p2 = -s "foo.cpp";
my $diff2 = $p2 - $p1;
print "improvement in program size: $diff2\n";
if ((10*$diff1) >= $diff2) {
    system "cp out.txt $DIR/out_best.txt";
    exit 0;
}
exit -1;
