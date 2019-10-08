// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

namespace NS {
  template <typename T1>
// CHECK: class A {};
// CHECK-SECOND: class AAA {};
  class AAA {};
}

template <typename T1>
// CHECK: class BBB: NS::A<T1> {};
// CHECK-SECOND: class A: NS::AAA<T1> {};
class BBB: NS::AAA<T1> {};
