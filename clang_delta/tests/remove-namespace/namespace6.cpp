// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-namespace --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

namespace NS1 { 
  void foo();
}
namespace NS2 {
  using namespace NS1;
  void bar() { NS1::foo(); }
}

// CHECK-FIRST-NOT:  namespace NS1 { 
// CHECK-FIRST:        void foo();
// CHECK-FIRST-NOT:  }
// CHECK-FIRST:      namespace NS2 {
// CHECK-FIRST-NOT:    using namespace NS1;
// CHECK-FIRST:        void bar() { foo(); }
// CHECK-FIRST:      }

// CHECK-SECOND:      namespace NS1 { 
// CHECK-SECOND:        void foo();
// CHECK-SECOND:      }
// CHECK-SECOND-NOT:  namespace NS2 {
// CHECK-SECOND-NOT:    using namespace NS1;
// CHECK-SECOND:        void bar() { NS1::foo(); }
// CHECK-SECOND-NOT:  }
