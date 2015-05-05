# Travis-CI is still on Precise: https://github.com/travis-ci/travis-ci/issues/2046
FROM ubuntu:precise

ENV CREDUCE /creduce
RUN mkdir $CREDUCE
WORKDIR $CREDUCE
COPY . $CREDUCE/

RUN scripts/travis_deps.sh
