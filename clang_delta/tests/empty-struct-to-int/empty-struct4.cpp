// RUN: %clang_delta --transformation=empty-struct-to-int --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: int g, i;
struct S {} g, i;
// CHECK: int s;
S s;
