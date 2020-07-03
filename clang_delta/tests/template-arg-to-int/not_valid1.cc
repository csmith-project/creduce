// RUN: %clang_delta --query-instances=template-arg-to-int %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: Available transformation instances: 0

template <typename T> struct S;
template <typename T1> struct A {
  S<T1> s; 
};
