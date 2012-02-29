--------------------------------------------------------------------

C-Reduce

--------------------------------------------------------------------
Description

--------------------------------------------------------------------
Prereqs

sudo apt-get install libfile-which-perl libregexp-common-perl indent astyle delta

--------------------------------------------------------------------
Building the clang_delta plugin

1. Install LLVM/Clang 3.0

2. Build the plugin:

cd clang_delta
export LLVM_PATH=/where/your/clang/installation/path
make


Note that assertions are enabled by default. To disable assertions,
build the program with:

LLVM_PATH=/path/ DISABLE_TRANS_ASSERT=1 make

--------------------------------------------------------------------

