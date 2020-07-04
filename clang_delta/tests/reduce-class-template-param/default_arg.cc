// RUN: %clang_delta --transformation=reduce-class-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=4 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FOURTH

template <typename T1, typename T2 = void>
struct S {};
struct S<int> s;
template <typename T1 = void, typename T2 = int >
struct S1 {};
struct S1<> t;

// CHECK-FIRST: template < typename T2 = void>
// CHECK-FIRST: struct S {};
// CHECK-FIRST: struct S<> s;
// CHECK-FIRST: template <typename T1 = void, typename T2 = int >
// CHECK-FIRST: struct S1 {};
// CHECK-FIRST: struct S1<> t;

// CHECK-SECOND: template <typename T1>
// CHECK-SECOND: struct S {};
// CHECK-SECOND: struct S<int> s;
// CHECK-SECOND: template <typename T1 = void, typename T2 = int >
// CHECK-SECOND: struct S1 {};
// CHECK-SECOND: struct S1<> t;

// CHECK-THIRD: template <typename T1, typename T2 = void>
// CHECK-THIRD: struct S {};
// CHECK-THIRD: struct S<int> s;
// CHECK-THIRD: template < typename T2 = int >
// CHECK-THIRD: struct S1 {};
// CHECK-THIRD: struct S1<> t;

// CHECK-FOURTH: template <typename T1, typename T2 = void>
// CHECK-FOURTH: struct S {};
// CHECK-FOURTH: struct S<int> s;
// CHECK-FOURTH: template <typename T1 = void >
// CHECK-FOURTH: struct S1 {};
// CHECK-FOURTH: struct S1<> t;

