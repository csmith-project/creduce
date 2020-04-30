// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-namespace --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=remove-namespace --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD

namespace NS1 {
  namespace NS2 { 
    namespace NS3 {
    } // NS3
  } // NS2
} // NS1
namespace c1 = NS1::NS2::NS3;

// CHECK-FIRST-NOT: namespace NS1 {
// CHECK-FIRST:       namespace NS2 { 
// CHECK-FIRST:         namespace NS3 {
// CHECK-FIRST:         } // NS3
// CHECK-FIRST:       } // NS2
// CHECK-FIRST-NOT: }
// CHECK-FIRST:     namespace c1 = NS2::NS3;

// CHECK-SECOND:     namespace NS1 {
// CHECK-SECOND-NOT:   namespace NS2 { 
// CHECK-SECOND:         namespace NS3 {
// CHECK-SECOND:         } // NS3
// CHECK-SECOND-NOT:   }
// CHECK-SECOND:     } // NS1
// CHECK-SECOND: namespace c1 = NS1::NS3;

// CHECK-THIRD:     namespace NS1 {
// CHECK-THIRD:       namespace NS2 { 
// CHECK-THIRD-NOT:     namespace NS3 {
// CHECK-THIRD-NOT:     }
// CHECK-THIRD:       } // NS2
// CHECK-THIRD:     } // NS1
// CHECK-THIRD: namespace c1 = NS1::NS2;

