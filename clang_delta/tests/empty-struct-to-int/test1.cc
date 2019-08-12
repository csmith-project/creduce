// RUN: %clang_delta --transformation=empty-struct-to-int --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK-NOT: struct S
struct S{};
// CHECK: int s1, s2;
S s1, s2;

