## -*- mode: Perl -*-

package pass_clang;

use strict;
use warnings;

use POSIX;
use File::Which;

use creduce_regexes;
use creduce_utils;

sub check_prereqs () {
    my $path = File::Which::which ("clang_delta");
    return defined ($path);
}

sub new ($$) {
    my $index = 1;
    return \$index;
}

sub advance ($$$) {
    (my $cfile, my $arg, my $state) = @_;
    my $index = ${$state};
    $index++;
    return \$index;
}

sub transform ($$$) {
    (my $cfile, my $which, my $state) = @_;
    my $index = ${$state};
    my $tmpfile = POSIX::tmpnam();
    my $cmd = "clang_delta --transformation=$which --counter=$index $cfile";
    my $res = runit ("$cmd > $tmpfile");
    if ($res==0) {
	system "mv $tmpfile $cfile";
	return ($OK, \$index);
    } else {
	if ($res == -1) {
	} else {
            my $tmpfile2 = $tmpfile;
            $tmpfile2 =~ s/\//_/g;
            $tmpfile2 = "clang_delta_crash${tmpfile2}.c";
	    system "cp $cfile $tmpfile2";
	    open TMPF, ">>$tmpfile2";
	    print TMPF "\n\n\/\/ $cmd\n";
	    close TMPF;
	    print "\n\n=======================================\n\n";
	    print "OOPS: clang_delta crashed; please consider\n";
	    print "mailing ${tmpfile2} to creduce-bugs\@flux.utah.edu\n";
	    print "and we will try to fix the bug\n";
	    print "\n=======================================\n\n";
	}
	system "rm $tmpfile";
	return ($STOP, \$index);
    }    
}

1;
