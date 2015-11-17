##
## Copyright (c) 2015 Tamir Duberstein
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.
##

###############################################################################

# Travis-CI is still on Precise: https://github.com/travis-ci/travis-ci/issues/2046

FROM ubuntu:precise

ENV CREDUCE /creduce
RUN mkdir $CREDUCE
WORKDIR $CREDUCE
COPY . $CREDUCE/

RUN scripts/travis_deps.sh
