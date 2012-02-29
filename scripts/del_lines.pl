#!/usr/bin/perl -w

use strict;
use File::Temp;

die unless (scalar(@ARGV)==3);

(my $fn, my $command, my $number) = @ARGV;

(my $TMP, my $tmpfilename) = File::Temp->tempfile("tempXXXXX", 
						  UNLINK => 0);
open INF, "<$fn" or die;
my $count=0;
while (my $line = <INF>) {
    if ($count == $number) {
    } else {
	print $TMP $line;
    }
    $count++;
}
close INF;
close $TMP;
print "$tmpfilename\n";
system "mv $tmpfilename $fn";
