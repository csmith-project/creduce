// RUN: %clang_delta --transformation=return-void --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: void foo(char *p) { }
char *foo(char *p) { }
