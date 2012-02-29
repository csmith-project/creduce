#!/usr/bin/perl -w

### this script preps a testcase for reduction and calls the result
### small.c; it takes the same arguments as Csmith

use strict;

my $CSMITH_HOME = $ENV{"CSMITH_HOME"};
die if (!defined($CSMITH_HOME));

my $outfile = "orig_csmith_output.c";
for (my $i=0; $i<scalar(@ARGV); $i++) {
    if ($ARGV[$i] eq "--output") {
	$outfile = $ARGV[$i+1];
    }
}
die if (!defined($outfile)); 

system "${CSMITH_HOME}/src/csmith @ARGV";

system "gcc -E -I${CSMITH_HOME}/runtime -DCSMITH_MINIMAL $outfile | ${CSMITH_HOME}/utah/scripts/reduce/shorten.pl > small.c";

system "indent small.c";
