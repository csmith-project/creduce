// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=rename-class --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD

// CHECK: struct A {};
struct S1 {};

// CHECK-SECOND: template<typename T> class A {
template<typename T> class Base {
// CHECK: int foo(A) {}
  int foo(S1) {}
};

// CHECK-SECOND: template<typename Tp, typename T> class S2 : A<Tp> {
// CHECK-THIRD: template<typename Tp, typename T> class A : Base<Tp> {
template<typename Tp, typename T> class S2 : Base<Tp> {
// CHECK: int foo(A) {}
  int foo(S1) {}
};

