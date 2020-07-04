// RUN: %clang_delta --transformation=reduce-class-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=4 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FOURTH

template <typename T1, typename T2, typename T3, typename T4>
struct S {};
struct S1 {};
struct S<long long, struct S1 *, unsigned int, unsigned short> s;

// CHECK-FIRST: template < typename T2, typename T3, typename T4>
// CHECK-FIRST: struct S {};
// CHECK-FIRST: struct S1 {};
// CHECK-FIRST: struct S< struct S1 *, unsigned int, unsigned short> s;

// CHECK-SECOND: template <typename T1,  typename T3, typename T4>
// CHECK-SECOND: struct S {};
// CHECK-SECOND: struct S1 {};
// CHECK-SECOND: struct S<long long,  unsigned int, unsigned short> s;

// CHECK-THIRD: template <typename T1, typename T2,  typename T4>
// CHECK-THIRD: struct S {};
// CHECK-THIRD: struct S1 {};
// CHECK-THIRD: struct S<long long, struct S1 *,  unsigned short> s;

// CHECK-FOURTH: template <typename T1, typename T2, typename T3>
// CHECK-FOURTH: struct S {};
// CHECK-FOURTH: struct S1 {};
// CHECK-FOURTH: struct S<long long, struct S1 *, unsigned int> s;

