#!/usr/bin/env sh
##
## Copyright (c) 2015 The University of Utah
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

# Set up for installing the LLVM development branch (aka "3.8").
# See <https://wiki.ubuntu.com/ToolChain>.
# See <http://llvm.org/apt/>.
add-apt-repository -y \
    ppa:ubuntu-toolchain-r/test
add-apt-repository -y \
    'deb http://llvm.org/apt/precise/ llvm-toolchain-precise main'
wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key | apt-key add -
apt-get update -qq

# Install LLVM development branch (aka "3.8").  See file "INSTALL".
apt-get install -y -qq \
    llvm-3.8 \
    llvm-3.8-dev \
    clang-3.8 \
    libclang-3.8-dev \
    clang-format-3.8 \
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
    libz-dev

###############################################################################

## End of file.
