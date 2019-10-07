// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

template<typename T1, typename T2>
// CHECK: struct A {
// CHECK-SECOND: struct AAA {
struct AAA {
  typedef T2 new_type;
};

template<typename T3>
// CHECK: struct BBB : public A<int, T3>::new_type { };
// CHECK-SECOND: struct A : public AAA<int, T3>::new_type { };
struct BBB : public AAA<int, T3>::new_type { };
