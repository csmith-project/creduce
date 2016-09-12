## -*- mode: Perl -*-
##
## Copyright (c) 2012, 2013, 2015, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

package pass_special;

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

    if ($which eq "a") {
      # special case for Csmith output
      $replace_cont = -1;
      $prog2 =~ s/(?<all>transparent_crc\s*\((?<list>.*?)\))/replace_aux($index,$+{all},junk($+{list}))/egs;
    } elsif ($which eq "b") {
      $replace_cont = -1;
      $prog2 =~ s/extern \"C\"/replace_aux($index,"extern \"C\"", "")/egs;
    } elsif ($which eq "c") {
      $replace_cont = -1;
      $prog2 =~ s/extern \"C\+\+\"/replace_aux($index,"extern \"C++\"", "")/egs;
    } else {
      die();
    }

    if ($prog ne $prog2) {
	write_file ($cfile, $prog2);
	return ($OK, \$index);
    } else {
	return ($STOP, \$index);
    }
}

1;
