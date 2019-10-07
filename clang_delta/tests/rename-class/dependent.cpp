// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

// CHECK: template <class T> struct A {};
// CHECK-SECOND: template <class T> struct Base {};
template <class T> struct Base {};
// CHECK: template <class T> struct Derived: public A<T> {
// CHECK-SECOND: template <class T> struct A: public Base<T> {
template <class T> struct Derived: public Base<T> {
// CHECK: typename Derived::template A<double>* p1;
// CHECK-SECOND: typename A::template Base<double>* p1;
  typename Derived::template Base<double>* p1;
};

