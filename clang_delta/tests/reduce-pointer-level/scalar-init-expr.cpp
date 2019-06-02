// RUN: %clang_delta --transformation=reduce-pointer-level --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct S
struct S {
// CHECK: typedef char c
  typedef char *c;
// CHECK: S() { c i = c(); }
  S() { c i = c(); }
};
