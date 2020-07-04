// RUN: %clang_delta --transformation=reduce-class-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

template<bool  , typename>
struct AAA { };
template <>
struct AAA<true ,  int > {};

// CHECK-FIRST: template< typename>
// CHECK-FIRST: struct AAA { };
// CHECK-FIRST: template <>
// CHECK-FIRST: struct AAA<  int > {};

// CHECK-SECOND: template<bool  >
// CHECK-SECOND: struct AAA { };
// CHECK-SECOND: template <>
// CHECK-SECOND: struct AAA<true  > {};

