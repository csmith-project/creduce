// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-namespace --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

namespace NS {
  namespace {
    int x;
  } // ANON
} // NS

// CHECK-FIRST-NOT:  namespace NS {
// CHECK-FIRST:        namespace {
// CHECK-FIRST:          int x;
// CHECK-FIRST:        } // ANON
// CHECK-FIRST-NOT:      }

// CHECK-SECOND:     namespace NS {
// CHECK-SECOND-NOT:   namespace {
// CHECK-SECOND:          int x;
// CHECK-SECOND-NOT:   }
// CHECK-SECOND:     } // NS

