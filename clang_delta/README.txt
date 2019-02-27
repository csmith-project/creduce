--------------------------------------------------------------------

Normally clang_delta is invoked from C-Reduce. But if you wish to
invoke it manually:

./clang_delta --transformation=xx --counter=xx foo.c

For detailed options:
./clang_delta --help

--------------------------------------------------------------------

Testing clang_delta:

* clang_delta/tests is the place for adding unittests for clang_delta
To run the tests, please follow the steps below:

If you use make: 
$ cd /creduce/top/dir
$ ./configure
$ make
$ cd clang_delta
$ make check-clang-delta

If you use cmake:
$ /creduce/top/dir
$ mkdir build
$ cd build
$ cmake ..
$ make
$ make check-clang-delta

* test_transformation
`test_transformation' is designed to test clang_delta
`test_transformation -help' gives detailed information.

--------------------------------------------------------------------

Known bugs: 

  * bad transformation on functions involved with function pointers

--------------------------------------------------------------------

