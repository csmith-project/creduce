// RUN: %clang_delta --transformation=reduce-class-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD

template<int I, typename Sign = unsigned, typename T = void>
struct S {};
template<int I, typename T>
struct S<I, signed, T *> {};

// CHECK-FIRST: template< typename Sign = unsigned, typename T = void>
// CHECK-FIRST: struct S {};
// CHECK-FIRST: template< typename T>
// CHECK-FIRST: struct S< signed, T *> {};

// CHECK-SECOND: template<int I,  typename T = void>
// CHECK-SECOND: struct S {};
// CHECK-SECOND: template<int I, typename T>
// CHECK-SECOND: struct S<I,  T *> {};

// CHECK-THIRD: template<int I, typename Sign = unsigned>
// CHECK-THIRD: struct S {};
// CHECK-THIRD: template<int I>
// CHECK-THIRD: struct S<I, signed> {};
