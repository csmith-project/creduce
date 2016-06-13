#!/usr/bin/env bash
##
## Copyright (c) 2012, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

# RUN THIS UNDER CDR FROM THE C-REDUCE MAIN DIR

# force all programs we use to be included
./tests/run_tests

# force all non-git files to be included
find . -name '*' | grep -v git | xargs wc
