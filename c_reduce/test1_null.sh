#!/bin/bash

# todo: these come from environment variables

CC1='gcc -w -O0'
CC2='gcc -w -O3 -funroll-loops -fomit-frame-pointer -fwhole-program'

rm -f out*.txt

if 
  echo here1 &&\
  current-gcc -Wall -Wextra -O small.c  >outa.txt 2>&1 &&\
  echo here2 &&\
  ! grep uninitialized outa.txt &&\
  ! grep 'control reaches end' outa.txt &&\
  ! grep 'ordered comparison of pointer with integer' outa.txt &&\
  ! grep 'incompatible implicit' outa.txt &&\
  ! grep 'excess elements in struct initializer' outa.txt &&\
  ! grep 'comparison between pointer and integer' outa.txt &&\
  clang -pedantic -Wall -O0 -c small.c  >out.txt 2>&1 &&\
  ! grep 'incompatible redeclaration' out.txt &&\
  ! grep 'ordered comparison between pointer' out.txt &&\
  ! grep 'eliding middle term' out.txt &&\
  ! grep 'end of non-void function' out.txt &&\
  ! grep 'invalid in C99' out.txt &&\
  ! grep 'should return a value' out.txt &&\
  ! grep uninitialized out.txt &&\
  ! grep 'incompatible pointer to' out.txt &&\
  ! grep 'incompatible integer to' out.txt &&\
  ! grep 'type specifier missing' out.txt &&\
  clang --analyze small.c > out_analyze.txt 2>&1 &&\
  ! grep garbage out_analyze.txt &&\
  ! grep undefined out_analyze.txt &&\
  echo here3 &&\
  $CC1 small.c -o small1 > cc_out1.txt 2>&1 &&\
  RunSafely.sh 3 1 /dev/null out1.txt ./small1 >/dev/null 2>&1 &&\
  valgrind -q --error-exitcode=1 ./small1 >/dev/null 2>&1 &&\
  echo here4 
then
  exit 0
else
  exit 1
fi
