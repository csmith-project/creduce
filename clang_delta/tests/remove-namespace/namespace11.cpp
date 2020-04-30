// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-namespace --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

namespace NS1 {
  void foo() {}
  void fiz() {}
} // NS1
namespace NS2 {
  using namespace NS1;
  int bar() { foo(); }
} // NS2
void fiz() {}
void f() { fiz(); }

// CHECK-FIRST-NOT:  namespace NS1 {
// CHECK-FIRST:        void foo() {}
// CHECK-FIRST:        void {{.*}}NS1_fiz() {}
// CHECK-FIRST-NOT:  }
// CHECK-FIRST:      namespace NS2 {
// CHECK-FIRST-NOT:    namespace NS1
// CHECK-FIRST:        int bar() { foo(); }
// CHECK-FIRST:      } // NS2
// CHECK-FIRST:      void fiz() {}
// CHECK-FIRST:      void f() { fiz(); }

// CHECK-SECOND:     namespace NS1 {
// CHECK-SECOND:       void foo() {}
// CHECK-SECOND:       void fiz() {}
// CHECK-SECOND:     } // NS1
// CHECK-SECOND-NOT: namespace NS2 {
// CHECK-SECOND-NOT:   namespace NS1
// CHECK-SECOND:       int bar() { NS1::foo(); }
// CHECK-SECOND-NOT: }
// CHECK-SECOND:     void fiz() {}
// CHECK-SECOND:     void f() { fiz(); }

