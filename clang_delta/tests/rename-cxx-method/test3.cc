// RUN: %clang_delta --transformation=rename-cxx-method --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

class A {
// CHECK: template < typename T > void m_fn1 (T) { m_fn1 <T> (0); }
  template < typename T > void foo (T) { foo <T> (0); }
};
