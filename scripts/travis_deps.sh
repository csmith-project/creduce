#!/usr/bin/env sh
##
## Copyright (c) 2015, 2016 The University of Utah
## Copyright (c) 2015 Tamir Duberstein
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.
##

###############################################################################

# This script is run under "sudo".  See file ".travis.yml".

set -eux

apt-get update -qq

# Install programs "add-apt-repository" and "wget", needed below.
apt-get install -y -qq \
    python-software-properties \
    wget

# Set up for installing LLVM 3.9.
# See <https://wiki.ubuntu.com/ToolChain>.
# See <http://llvm.org/apt/>.
add-apt-repository -y \
    ppa:ubuntu-toolchain-r/test
add-apt-repository -y \
    'deb http://apt.llvm.org/precise/ llvm-toolchain-precise-3.9 main'
wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
apt-get update -qq

# Install LLVM 3.9.  See file "INSTALL".
apt-get install -y --force-yes -qq \
    llvm-3.9 \
    llvm-3.9-dev \
    clang-3.9 \
    libclang-3.9-dev \
    clang-format-3.9 \
    libedit-dev

# Install other C-Reduce dependencies.  See file "INSTALL".
apt-get install -y -qq \
    indent \
    astyle \
    libexporter-lite-perl \
    libfile-which-perl \
    libgetopt-tabular-perl \
    libregexp-common-perl \
    libsys-cpu-perl \
    libterm-readkey-perl \
    flex \
    build-essential \
    zlib1g-dev

###############################################################################

## End of file.
