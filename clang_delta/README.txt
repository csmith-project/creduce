--------------------------------------------------------------------

Normally clang_delta is invoked from C-Reduce. But if you wish to
invoke it manually:

./clang_delta --transformation=xx --counter=xx foo.c

For detailed options:
./clang_delta --help

`test_transformation' is designed to test clang_delta
`test_transformation -help' gives detailed information.

--------------------------------------------------------------------

Known bugs: 

  * bad transformation on functions involved with function pointers

--------------------------------------------------------------------

