// RUN: %clang_delta --transformation=rename-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: void foo(int p1, [p1]);
void foo(int b, [b]);
