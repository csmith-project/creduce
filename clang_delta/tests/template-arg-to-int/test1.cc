// RUN: %clang_delta --transformation=template-arg-to-int --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct S {};
struct S {};
// CHECK: template <typename T> struct C {};
template <typename T> struct C {};
// CHECK: C<int> c;
C<S> c;

