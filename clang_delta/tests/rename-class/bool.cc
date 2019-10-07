// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

namespace NS1 {
// CHECK: template<typename T> class A {};
  template<typename T> class AAA {};
}

// CHECK: class BBB : public NS1::A<bool> {};
// CHECK-SECOND: class A : public NS1::AAA<bool> {};
class BBB : public NS1::AAA<bool> {};
