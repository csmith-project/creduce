use strict;
use warnings;

use lib '@perllibdir@';

use creduce_config qw(PACKAGE_STRING);
use creduce_utils;

my $num_args = $#ARGV + 1;

if ($num_args != 3) {
    exit (0);
}

my $test = $ARGV[0];
my $fn = $ARGV[1];
my $VERBOSE = $ARGV[2];
my $res;

if ($VERBOSE) {
    $res = runit "$test $fn";
} else {
    $res = runit "$test $fn > NUL 2>&1";
}

exit (($res == 0) ? 1 : 0);