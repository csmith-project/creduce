#!/bin/bash

ulimit -t 1

gcc -O1 small.c -o small1 >/dev/null 2>&1 &&\
./small1 > out1.txt &&\
gcc -O2 small.c -o small2 >/dev/null 2>&1 &&\
./small2 > out2.txt &&\
! diff out1.txt out2.txt
