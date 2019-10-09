// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

namespace NS {
// CHECK: template<bool C> struct A;
  template<bool C> struct S1;
// CHECK: typedef A<false> S1_t;
  typedef S1<false> S1_t;

// CHECK: template<bool C> struct A {
  template<bool C> struct S1 {
// CHECK: typedef A type;
    typedef S1 type;
  };
}

// CHECK-SECOND: struct A : NS::S1_t {};
struct S2 : NS::S1_t {};
