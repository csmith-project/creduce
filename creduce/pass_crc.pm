## -*- mode: Perl -*-

package pass_crc;

use strict;
use warnings;

use creduce_regexes;
use creduce_utils;

sub check_prereqs () {
    return 1;
}

sub new ($$) {
    my $index = 0;
    return \$index;
}

sub advance ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my $index = ${$state};
    $index++;
    return \$index;
}

sub junk ($) {
    (my $list) = @_;
    my @stuff = split /,/, $list;
    my $var = $stuff[0];
    return "printf (\"%d\\n\", (int)$var)";
}

sub transform ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my $index = ${$state};

    my $prog = read_file ($cfile);
    my $prog2 = $prog;

    # this only makes sense for Csmith output...
    $replace_cont = -1;
    $prog2 =~ s/(?<all>transparent_crc\s*\((?<list>.*?)\))/replace_aux($index,$+{all},junk($+{list}))/egs;

    if ($prog ne $prog2) {
	write_file ($cfile, $prog2);
	return ($OK, \$index);
    } else {
	return ($STOP, \$index);
    }
}

1;
