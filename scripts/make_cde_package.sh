#!/bin/bash

# RUN THIS UNDER CDR FROM THE C-REDUCE MAIN DIR

# force all programs we use to be included
./tests/run_tests

# force all non-git files to be included
find . -name '*' | grep -v git | xargs wc
