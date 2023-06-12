#!/bin/bash

gcc -c -O0 small.c &&\
! gcc -c -O3 small.c
