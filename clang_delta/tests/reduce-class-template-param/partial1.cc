// RUN: %clang_delta --transformation=reduce-class-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

template<bool  , typename>
struct AAA { };
template<typename T>
struct AAA<true, T *>
{ };
template <>
struct AAA<false, int> {};

// CHECK-FIRST: template< typename>
// CHECK-FIRST: struct AAA { };
// CHECK-FIRST: template<typename T>
// CHECK-FIRST: struct AAA< T *>
// CHECK-FIRST: { };
// CHECK-FIRST: template <>
// CHECK-FIRST: struct AAA< int> {};

// CHECK-SECOND: template<bool  >
// CHECK-SECOND: struct AAA { };
// CHECK-SECOND: template<>
// CHECK-SECOND: struct AAA<true>
// CHECK-SECOND: { };
// CHECK-SECOND: template <>
// CHECK-SECOND: struct AAA<false> {};
