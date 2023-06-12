# Installing C-Reduce

## Using a Package Manager

Before compiling C-Reduce yourself, you might want to see if your OS
comes with a precompiled package for C-Reduce.  Ubuntu, Debian, Gentoo,
FreeBSD and Mac OS X (Homebrew) all do.  For example, on OS X:

```
$ brew install creduce
```

## From Source

### Prereqs

C-Reduce is written in Perl, C++, and C.  To compile and run C-Reduce,
you will need a development environment that supports these languages.
C-Reduce's build system requires CMake or GNU Make (*not* BSD Make).

Beyond the basic compile/build tools, C-Reduce depends on a set of
third-party software packages, including LLVM.

On Ubuntu or Mint, the prerequisites other than LLVM can be installed
like this:

```
sudo apt-get install \
  libexporter-lite-perl libfile-which-perl libgetopt-tabular-perl \
  libregexp-common-perl flex build-essential zlib1g-dev
```

On OS X, [Perlbrew](http://perlbrew.pl/) provides an easy and flexible
way to get Perl and Perl modules installed

Otherwise, install these packages either manually or using the package
manager:

* [Flex](http://flex.sourceforge.net/)

* [LLVM+Clang 16.0.5](https://github.com/llvm/llvm-project/releases/download/llvmorg-16.0.5/llvm-project-16.0.5.src.tar.xz)
  Either compile this from source or else, if possible, grab a
  pre-built binary that is appropriate for your system.

* Perl modules:
  - Exporter::Lite
  - File::Which
  - Getopt::Tabular
  - Regexp::Common
  - For example, (perhaps as root) `cpan -i Exporter::Lite File::Which Getopt::Tabular Regexp::Common`

* [zlib](http://www.zlib.net/)

### Optional Prereqs

`Term::ReadKey` is optional; C-Reduce will use it if it is installed.

On Ubuntu:

```
sudo apt-get install libterm-readkey-perl
```

On OS X (with Homebrew + Perlbrew installed):

```
cpan -i Term::ReadKey
```

Otherwise, install the packages either manually or using the package
manager.

## Building and installing C-Reduce

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

