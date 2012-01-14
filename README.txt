(1) How to build it:

llvm_installation_dir=/where/your/clang/installation/path make
e.g., 
llvm_installation_dir=/uusoc/exports/scratch/chenyang/clang_reducer/llvm-3.0

(2) how to run:

./clang_delta --transformation=xx --counter=xx foo.c

For detailed options:
./clang_delta --help

(3) Known problems:
  * bad transformation on functions invovled with function pointers

