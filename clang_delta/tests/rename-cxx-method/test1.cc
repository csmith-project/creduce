// RUN: %clang_delta --transformation=rename-cxx-method --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: class A { template<typename T> void m_fn1(T p) {} };
class A { template<typename T> void foo(T p) {} };
