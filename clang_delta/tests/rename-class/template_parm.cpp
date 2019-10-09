// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

template<typename T1> 
// CHECK: class A {
class AAA {
// CHECK-SECOND: struct A : T1 {};
  struct BBB : T1 {};
};
