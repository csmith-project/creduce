// RUN: %clang_delta --transformation=remove-unused-function --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct S {};
struct S {};
// CHECK-NEXT:{{^ *; *$}}
template<typename T>
S foo(T &, T &) = delete;
