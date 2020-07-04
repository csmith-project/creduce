// RUN: %clang_delta --transformation=reduce-class-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

template<typename T1, typename T2> struct A {
  template <typename U1, typename U2> struct B {
    U1 foo(U2 p);
  };
};

// CHECK-FIRST: template< typename T2> struct A {
// CHECK-FIRST:   template <typename U1, typename U2> struct B {
// CHECK-FIRST:     U1 foo(U2 p);
// CHECK-FIRST:   };
// CHECK-FIRST: };

// CHECK-SECOND: template<typename T1> struct A {
// CHECK-SECOND:   template <typename U1, typename U2> struct B {
// CHECK-SECOND:     U1 foo(U2 p);
// CHECK-SECOND:   };
// CHECK-SECOND: };

