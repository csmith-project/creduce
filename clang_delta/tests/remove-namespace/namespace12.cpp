// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-namespace --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=remove-namespace --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD

namespace NS1 { }
namespace NS2 {
  namespace NS1 {
    void foo() {}
  } // NS1
  namespace NS3 {
    using namespace NS1;
    void bar() {foo();}
  } // NS3
} // NS2

// CHECK-FIRST-NOT:  namespace NS1 { }
// CHECK-FIRST:      namespace NS2 {
// CHECK-FIRST:        namespace NS1 {
// CHECK-FIRST:          void foo() {}
// CHECK-FIRST:        } // NS1
// CHECK-FIRST:        namespace NS3 {
// CHECK-FIRST:          using namespace NS1;
// CHECK-FIRST:          void bar() {foo();}
// CHECK-FIRST:        } // NS3
// CHECK-FIRST:      } // NS2

// CHECK-SECOND:     namespace NS1 { }
// CHECK-SECOND-NOT:   namespace NS2 {
// CHECK-SECOND:       namespace [[NEWNS:[a-zA-Z_]+NS2_NS1]] {
// CHECK-SECOND:          void foo() {}
// CHECK-SECOND:        } // NS1
// CHECK-SECOND:        namespace NS3 {
// CHECK-SECOND:          using namespace [[NEWNS]];
// CHECK-SECOND:          void bar() {foo();}
// CHECK-SECOND:        } // NS3
// CHECK-SECOND-NOT:  }

// CHECK-THIRD:       namespace NS1 { }
// CHECK-THIRD:       namespace NS2 {
// CHECK-THIRD-NOT:     namespace NS1 {
// CHECK-THIRD:           void foo() {}
// CHECK-THIRD-NOT:     }
// CHECK-THIRD:        namespace NS3 {
// CHECK-THIRD-NOT:      namespace NS1;
// CHECK-THIRD:          void bar() {foo();}
// CHECK-THIRD:        } // NS3
// CHECK-THIRD:      } // NS2

