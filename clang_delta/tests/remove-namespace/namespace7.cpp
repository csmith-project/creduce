// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-namespace --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=remove-namespace --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD

namespace NS1 {
  namespace NS2 {
    void foo() {}
  } // NS2
} // NS1
namespace NS3 {
  using namespace NS1;
  void bar() { NS2::foo(); }
} // NS3

// CHECK-FIRST-NOT:  namespace NS1 {
// CHECK-FIRST:        namespace NS2 {
// CHECK-FIRST:          void foo() {}
// CHECK-FIRST:        } // NS2
// CHECK-FIRST-NOT:  }
// CHECK-FIRST:      namespace NS3 {
// CHECK-FIRST-NOT:    NS1;
// CHECK-FIRST:        void bar() { NS2::foo(); }
// CHECK-FIRST:      } // NS3

// CHECK-SECOND:     namespace NS1 {
// CHECK-SECOND-NOT:   namespace NS2 {
// CHECK-SECOND:          void foo() {}
// CHECK-SECOND-NOT:   }
// CHECK-SECOND:     } // NS1
// CHECK-SECOND:     namespace NS3 {
// CHECK-SECOND:       using namespace NS1;
// CHECK-SECOND:       void bar() { foo(); }
// CHECK-SECOND:     } // NS3

// CHECK-THIRD:      namespace NS1 {
// CHECK-THIRD:        namespace NS2 {
// CHECK-THIRD:          void foo() {}
// CHECK-THIRD:        } // NS2
// CHECK-THIRD:      } // NS1
// CHECK-THIRD-NOT:  namespace NS3 {
// CHECK-THIRD-NOT:    NS1;
// CHECK-THIRD:        void bar() { NS1::NS2::foo(); }
// CHECK-THIRD-NOT:  }

