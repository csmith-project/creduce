// RUN: %clang_delta --transformation=reduce-class-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

template<typename T1, typename T2 = void>
struct S {};
template<typename T1>
struct S<T1 *> {};

// CHECK-FIRST: template< typename T2 = void>
// CHECK-FIRST: struct S {};
// CHECK-FIRST: template<>
// CHECK-FIRST: struct S<> {};

// CHECK-SECOND: template<typename T1>
// CHECK-SECOND: struct S {};
// CHECK-SECOND: template<typename T1>
// CHECK-SECOND: struct S<T1 *> {};
