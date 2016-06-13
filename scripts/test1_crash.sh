#!/usr/bin/env bash
##
## Copyright (c) 2012, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

rm -f out*.txt

ulimit -t 1
ulimit -v 2000000

if 
  clang -pedantic -Wall -O0 -c small.c  >out.txt 2>&1 &&\
  ! grep 'incompatible redeclaration' out.txt &&\
  ! grep 'ordered comparison between pointer' out.txt &&\
  ! grep 'eliding middle term' out.txt &&\
  ! grep 'end of non-void function' out.txt &&\
  ! grep 'invalid in C99' out.txt &&\
  ! grep 'specifies type' out.txt &&\
  ! grep 'should return a value' out.txt &&\
  ! grep 'too few argument' out.txt &&\
  ! grep 'too many argument' out.txt &&\
  ! grep "return type of 'main" out.txt &&\
  ! grep 'uninitialized' out.txt &&\
  ! grep 'incompatible pointer to' out.txt &&\
  ! grep 'incompatible integer to' out.txt &&\
  ! grep 'type specifier missing' out.txt &&\
  gcc -c -Wall -Wextra -O1 small.c  >outa.txt 2>&1 &&\
  ! grep uninitialized outa.txt &&\
  ! grep 'control reaches end' outa.txt &&\
  ! grep 'no semicolon at end' outa.txt &&\
  ! grep 'incompatible pointer' outa.txt &&\
  ! grep 'cast from pointer to integer' outa.txt &&\
  ! grep 'ordered comparison of pointer with integer' outa.txt &&\
  ! grep 'declaration does not declare anything' outa.txt &&\
  ! grep 'expects type' outa.txt &&\
  ! grep 'assumed to have one element' outa.txt &&\
  ! grep 'division by zero' outa.txt &&\
  ! grep 'pointer from integer' outa.txt &&\
  ! grep 'incompatible implicit' outa.txt &&\
  ! grep 'excess elements in struct initializer' outa.txt &&\
  ! grep 'comparison between pointer and integer' outa.txt &&\
  ! XXOPT1 small.c -o small1 > cc_out1.txt 2>&1 &&\
  grep XXFAIL cc_out1.txt
then
  exit 0
else
  exit 1
fi

#  clang --analyze small.c > out_analyze.txt 2>&1 &&\
#  ! grep garbage out_analyze.txt &&\
#  ! grep undefined out_analyze.txt &&\
