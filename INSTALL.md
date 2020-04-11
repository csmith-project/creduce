# Installing C-Reduce

## Using a Package Manager

Before compiling C-Reduce yourself, you might want to see if your OS
comes with a precompiled package for C-Reduce.  Ubuntu, Debian, Gentoo,
and Mac OS X (Homebrew) all do.  For example, on OS X:

```
$ brew install creduce
```

## From Source

### Prereqs

C-Reduce is written in Python3, C++, and C.  To compile and run C-Reduce,
you will need a development environment that supports these languages.
C-Reduce's build system requires CMake.

Beyond the basic compile/build tools, C-Reduce depends on a set of
third-party software packages, including LLVM.

On Ubuntu or Mint, the prerequisites other than LLVM can be installed
like this:

```
sudo apt-get install \
  flex build-essential
```

On FreeBSD 11.2, the prerequisites can be installed like this:

```
sudo pkg install \
  llvm90 flex
```

Otherwise, install these packages either manually or using the package
manager:

* [Flex](http://flex.sourceforge.net/)

* [LLVM/Clang 9.0.0](http://llvm.org/releases/download.html#9.0.0)
  (No need to compile it: the appropriate "pre-built binaries" package is
  all you need.  If you use one of the binary packages, you may need
  to install additional packages that the binary package depends on.
  For example, the "Ubuntu 18.04" binary package depends on "libtinfo5"
  and "libxml2".  You may need to install these, e.g.:
  `sudo apt-get install libtinfo-dev libxml2-dev`.)

* [Python3](https://www.python.org/downloads/)

* [CMake](https://cmake.org/)

## Building and installing C-Reduce

You can configure, build, and install C-Reduce with the CMake.

From either the source directory or a build directory:

```
cmake [source-dir] [options]
make
make install
```

If LLVM/Clang is not in your search path, you can tell CMake where to
find LLVM/Clang:

```
# Use the LLVM/Clang tree rooted at /opt/llvm
cmake [source-dir] -DCMAKE_PREFIX_PATH=/opt/llvm
```

Alternatively, if you choose to build LLVM and Clang yourself, you can
set the `LLVM_DIR` and/or `Clang_DIR` variables to paths where CMake can
find the `LLVMConfig.cmake` and/or `ClangConfig.cmake` files.  The
value of `LLVM_DIR` is usually `./lib/cmake/llvm`, relative to your LLVM
build or install directory.  Similarly, the value of `Clang_DIR` is
usually `./lib/cmake/clang`, relative to your Clang build or install
directory.  For example:

```
# Use separate LLVM and Clang build trees, /work/my-{llvm,clang}
cmake [source-dir] -DLLVM_DIR=/work/my-llvm/lib/cmake/llvm \
  -DClang_DIR=/work/my-clang/lib/cmake/clang
```

You do *not* need to set `Clang_DIR` if you build Clang within your LLVM
tree.  Also, note that you must actually *build* LLVM and Clang before
building C-Reduce.

Note that assertions are enabled by default. To disable assertions:

```
cmake ... -DENABLE_TRANS_ASSERT=OFF
```

## Regarding LLVM versions

Released versions of C-Reduce, and also our master branch at GitHub,
need to be compiled against specific released versions of LLVM, as
noted in this file.
