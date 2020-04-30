// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-namespace --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

namespace NS1 {
  namespace NS2 {
    void foo();
  } // NS2
} // NS1
namespace cl = NS1::NS2;
void bar() {cl::foo(); }

// CHECK-FIRST-NOT:  namespace NS1 {
// CHECK-FIRST:        namespace NS2 {
// CHECK-FIRST:          void foo();
// CHECK-FIRST:        } // NS2
// CHECK-FIRST-NOT:  }
// CHECK-FIRST:      namespace cl = NS2;
// CHECK-FIRST:      void bar() {cl::foo(); }

// CHECK-SECOND:     namespace NS1 {
// CHECK-SECOND-NOT:   namespace NS2 {
// CHECK-SECOND:         void foo();
// CHECK-SECOND-NOT:   }
// CHECK-SECOND:     } // NS1
// CHECK-SECOND:     namespace cl = NS1;
// CHECK-SECOND:     void bar() {cl::foo(); }

