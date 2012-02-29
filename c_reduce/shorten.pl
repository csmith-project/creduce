#!/usr/bin/perl -w
##
## Copyright (c) 2011 The University of Utah
## All rights reserved.
##
## This file is part of `csmith', a random generator of C programs.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
##   * Redistributions of source code must retain the above copyright notice,
##     this list of conditions and the following disclaimer.
##
##   * Redistributions in binary form must reproduce the above copyright
##     notice, this list of conditions and the following disclaimer in the
##     documentation and/or other materials provided with the distribution.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.

use strict;

my @l = ("safe_unary_minus_func_int8_t_s",
	 "safe_add_func_int8_t_s_s",
	 "safe_sub_func_int8_t_s_s",
	 "safe_mul_func_int8_t_s_s",
	 "safe_mod_func_int8_t_s_s",
	 "safe_div_func_int8_t_s_s",
	 "safe_lshift_func_int8_t_s_s",
	 "safe_lshift_func_int8_t_s_u",
	 "safe_rshift_func_int8_t_s_s",
	 "safe_rshift_func_int8_t_s_u",
	 "safe_unary_minus_func_int16_t_s",
	 "safe_add_func_int16_t_s_s",
	 "safe_sub_func_int16_t_s_s",
	 "safe_mul_func_int16_t_s_s",
	 "safe_mod_func_int16_t_s_s",
	 "safe_div_func_int16_t_s_s",
	 "safe_lshift_func_int16_t_s_s",
	 "safe_lshift_func_int16_t_s_u",
	 "safe_rshift_func_int16_t_s_s",
	 "safe_rshift_func_int16_t_s_u",
	 "safe_unary_minus_func_int32_t_s",
	 "safe_add_func_int32_t_s_s",
	 "safe_sub_func_int32_t_s_s",
	 "safe_mul_func_int32_t_s_s",
	 "safe_mod_func_int32_t_s_s",
	 "safe_div_func_int32_t_s_s",
	 "safe_lshift_func_int32_t_s_s",
	 "safe_lshift_func_int32_t_s_u",
	 "safe_rshift_func_int32_t_s_s",
	 "safe_rshift_func_int32_t_s_u",
	 "safe_unary_minus_func_int64_t_s",
	 "safe_add_func_int64_t_s_s",
	 "safe_sub_func_int64_t_s_s",
	 "safe_mul_func_int64_t_s_s",
	 "safe_mod_func_int64_t_s_s",
	 "safe_div_func_int64_t_s_s",
	 "safe_lshift_func_int64_t_s_s",
	 "safe_lshift_func_int64_t_s_u",
	 "safe_rshift_func_int64_t_s_s",
	 "safe_rshift_func_int64_t_s_u",
	 "safe_unary_minus_func_uint8_t_u",
	 "safe_add_func_uint8_t_u_u",
	 "safe_sub_func_uint8_t_u_u",
	 "safe_mul_func_uint8_t_u_u",
	 "safe_mod_func_uint8_t_u_u",
	 "safe_div_func_uint8_t_u_u",
	 "safe_lshift_func_uint8_t_u_s",
	 "safe_lshift_func_uint8_t_u_u",
	 "safe_rshift_func_uint8_t_u_s",
	 "safe_rshift_func_uint8_t_u_u",
	 "safe_unary_minus_func_uint16_t_u",
	 "safe_add_func_uint16_t_u_u",
	 "safe_sub_func_uint16_t_u_u",
	 "safe_mul_func_uint16_t_u_u",
	 "safe_mod_func_uint16_t_u_u",
	 "safe_div_func_uint16_t_u_u",
	 "safe_lshift_func_uint16_t_u_s",
	 "safe_lshift_func_uint16_t_u_u",
	 "safe_rshift_func_uint16_t_u_s",
	 "safe_rshift_func_uint16_t_u_u",
	 "safe_unary_minus_func_uint32_t_u",
	 "safe_add_func_uint32_t_u_u",
	 "safe_sub_func_uint32_t_u_u",
	 "safe_mul_func_uint32_t_u_u",
	 "safe_mod_func_uint32_t_u_u",
	 "safe_div_func_uint32_t_u_u",
	 "safe_lshift_func_uint32_t_u_s",
	 "safe_lshift_func_uint32_t_u_u",
	 "safe_rshift_func_uint32_t_u_s",
	 "safe_rshift_func_uint32_t_u_u",
	 "safe_unary_minus_func_uint64_t_u",
	 "safe_add_func_uint64_t_u_u",
	 "safe_sub_func_uint64_t_u_u",
	 "safe_mul_func_uint64_t_u_u",
	 "safe_mod_func_uint64_t_u_u",
	 "safe_div_func_uint64_t_u_u",
	 "safe_lshift_func_uint64_t_u_s",
	 "safe_lshift_func_uint64_t_u_u",
	 "safe_rshift_func_uint64_t_u_s",
	 "safe_rshift_func_uint64_t_u_u",
    );

my %ren;

my $n=0;
foreach my $s (@l) {
    $n++;
    $ren{$s} = "safe_$n";
}

my $prog = "";

while (my $line = <STDIN>) { 
    chomp $line;
    if ($line ne "" &&
	!($line =~ /^#/)) {
	$prog .= "$line\n"; 
    }
}

foreach my $s (@l) {
    my $s2 = $ren{$s};
    ($prog =~ s/$s/$s2/g);
}

print $prog;

