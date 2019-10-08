// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=rename-class --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD

// CHECK: struct A {};
// CHECK-SECOND: struct XXX {};
// CHECK-THIRD: struct XXX {};
struct XXX {};

template<typename T1, typename T2>
// CHECK-SECOND: struct A {
// CHECK-THIRD: struct AAA {
struct AAA {
  typedef T2 new_type;
};

// CHECK: struct BBB : public AAA<int, A>::new_type
// CHECK-SECOND: struct BBB : public A<int, XXX>::new_type
// CHECK-THIRD: struct A : public AAA<int, XXX>::new_type
struct BBB : public AAA<int, XXX>::new_type
{ };
