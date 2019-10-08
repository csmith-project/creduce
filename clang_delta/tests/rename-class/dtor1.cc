// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

template<typename T>
// CHECK: class A {
// CHECK-SECOND: class AAA {
class AAA {
// CHECK: ~A();
  ~ AAA();
};

// CHECK: class HHH : A<int> {
// CHECK-SECOND: class A : AAA<int> {
class HHH : AAA<int> {
};

