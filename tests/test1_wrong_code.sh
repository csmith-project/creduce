#!/bin/bash

rm -f out*.txt

if 
  echo here0 &&\
  gcc -Wall -Wextra -O2 small.c -o smallz >outa.txt 2>&1 &&\
  ! grep uninitialized outa.txt &&\
  ! grep 'control reaches end' outa.txt &&\
  ! grep 'no semicolon at end' outa.txt &&\
  ! grep 'incompatible pointer' outa.txt &&\
  ! grep 'ordered comparison of pointer with integer' outa.txt &&\
  ! grep 'declaration does not declare anything' outa.txt &&\
  ! grep 'arguments for format' outa.txt &&\
  ! grep 'expects type' outa.txt &&\
  ! grep 'pointer from integer' outa.txt &&\
  ! grep 'incompatible implicit' outa.txt &&\
  ! grep 'excess elements in struct initializer' outa.txt &&\
  ! grep 'comparison between pointer and integer' outa.txt &&\
  clang -pedantic -Wall -O0 -c small.c  >out.txt 2>&1 &&\
  ! grep 'conversions than data arguments' out.txt &&\
  ! grep 'incompatible redeclaration' out.txt &&\
  ! grep 'ordered comparison between pointer' out.txt &&\
  ! grep 'eliding middle term' out.txt &&\
  ! grep 'end of non-void function' out.txt &&\
  ! grep 'invalid in C99' out.txt &&\
  ! grep 'specifies type' out.txt &&\
  ! grep 'should return a value' out.txt &&\
  ! grep 'uninitialized' out.txt &&\
  ! grep 'incompatible pointer to' out.txt &&\
  ! grep 'incompatible integer to' out.txt &&\
  ! grep 'type specifier missing' out.txt &&\
  echo here1 &&\
  clang --analyze small.c > out_analyze.txt 2>&1 &&\
  ! grep garbage out_analyze.txt &&\
  ! grep undefined out_analyze.txt &&\
  echo here2 &&\
  RunSafely.sh 30 1 /dev/null out_vg.txt valgrind -q --error-exitcode=1 ./smallz &&\
  echo here3 &&\
  cp small.c small-framac.c &&\
  perl -pi.bak -e 's/int main \(int argc, char\* argv\[\]\)/int argc; char **argv; int main (void)/' small-framac.c &&\
  RunSafely.sh 125 1 /dev/null out_framac.txt frama-c -cpp-command \"gcc -C -Dvolatile= -E -I.\" -val-signed-overflow-alarms -val -stop-at-first-alarm -no-val-show-progress -machdep x86_64 -obviously-terminates -precise-unions small-framac.c &&\
  ! grep assert out_framac.txt &&\
  echo here4 &&\
  XXOPT1 small.c -o small1 > cc_out1.txt 2>&1 &&\
  RunSafely.sh 3 1 /dev/null out1.txt ./small1 >/dev/null 2>&1 &&\
  XXOPT2 small.c -o small2 > cc_out2.txt 2>&1 &&\
  RunSafely.sh 3 1 /dev/null out2.txt ./small2 >/dev/null 2>&1 &&\
  ! diff out1.txt out2.txt 
then
  echo here5;
  exit 0
else
  echo here6;
  exit 1
fi
