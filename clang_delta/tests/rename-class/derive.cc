// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

// CHECK: class A {};
class S1 {};
// CHECK: class S2:A {
// CHECK-SECOND: class A:S1 {
class S2:S1 {
public:
// CHECK-SECOND: A () {}
  S2 () {}
};
