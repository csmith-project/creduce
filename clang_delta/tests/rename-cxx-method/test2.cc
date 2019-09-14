// RUN: %clang_delta --transformation=rename-cxx-method --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

class A {
// CHECK: template <typename T> static int m_fn1(T, int) {
  template <typename T> static int foo(T, int) {
// CHECK: m_fn1<T, 0>(0, 0);
     foo<T, 0>(0, 0);
  }
};
