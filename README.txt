--------------------------------------------------------------------

C-Reduce

--------------------------------------------------------------------
Description

--------------------------------------------------------------------
Prereqs

sudo apt-get install libfile-which-perl libregexp-common-perl indent astyle delta

--------------------------------------------------------------------
Installation

(1) How to build clang_delta:

First make sure you have installed Clang 3.0, then

LLVM_PATH=/where/your/clang/installation/path make
e.g., 
LLVM_PATH=/uusoc/exports/scratch/chenyang/clang_reducer/llvm-3.0 make

Note that assertions are enabled by default. To disable assertions,
build the program with:

LLVM_PATH=/path/ DISABLE_TRANS_ASSERT=1 make

(2) How to run:

./clang_delta --transformation=xx --counter=xx foo.c

For detailed options:
./clang_delta --help

(3) test_transformation.pl is designed to test clang_delta
"test_transformation.pl -help" gives detailed information.

(4) Known problems:
  * bad transformation on functions invovled with function pointers

--------------------------------------------------------------------

