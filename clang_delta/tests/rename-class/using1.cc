// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

namespace NS {
// CHECK: struct A { static void foo(); };
  struct XYZ { static void foo(); };
// CHECK: void A::foo() {}
  void XYZ::foo() {}
}

// CHECK: using NS::A;
using NS::XYZ;
void bar(void) {
// CHECK: NS::A::foo();
  NS::XYZ::foo();
}
