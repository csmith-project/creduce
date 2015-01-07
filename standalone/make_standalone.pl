#!/usr/bin/perl -w

use strict;

my @deps = ("diff", "indent", "astyle", "topformflat", "clang_format");
my @progs = ("clang_delta", "clex", "strlex");

my $target = $ARGV[0];

