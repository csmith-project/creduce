// RUN: %clang_delta --transformation=param-to-local --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=param-to-local --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

int foo(int p1, int p2);

// CHECK-FIRST: int foo( int p2);

// CHECK-SECOND: int foo(int p1);

