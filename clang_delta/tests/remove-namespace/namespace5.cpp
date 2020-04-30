// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-namespace --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=remove-namespace --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD
// RUN: %clang_delta --transformation=remove-namespace --counter=4 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FOURTH

namespace NS1 { }
namespace NS2 {
  namespace NS1 {
    void foo() {}
  }
  namespace NS3 {
    using NS1::foo;
    void bar() { foo(); }
  }
}

// CHECK-FIRST-NOT:  namespace NS1 { }
// CHECK-FIRST:      namespace NS2 {
// CHECK-FIRST:        namespace NS1 {
// CHECK-FIRST:          void foo() {}
// CHECK-FIRST:        }
// CHECK-FIRST:        namespace NS3 {
// CHECK-FIRST:          using NS1::foo;
// CHECK-FIRST:          void bar() { foo(); }
// CHECK-FIRST:        }
// CHECK-FIRST:      }

// CHECK-SECOND:     namespace NS1 { }
// CHECK-SECOND-NOT: namespace NS2 {
// CHECK-SECOND:       namespace [[VNS1:[a-zA-Z_]+NS2_NS1]] {
// CHECK-SECOND:         void foo() {}
// CHECK-SECOND:       }
// CHECK-SECOND:       namespace NS3 {
// CHECK-SECOND:         using [[VNS1]]::foo;
// CHECK-SECOND:         void bar() { foo(); }
// CHECK-SECOND:       }
// CHECK-SECOND-NOT: }

// CHECK-THIRD:      namespace NS1 { }
// CHECK-THIRD:      namespace NS2 {
// CHECK-THIRD-NOT:    namespace NS1 {
// CHECK-THIRD:          void foo() {}
// CHECK-THIRD-NOT:    }
// CHECK-THIRD:        namespace NS3 {
// CHECK-THIRD-NOT:      using NS1::foo;
// CHECK-THIRD:          void bar() { foo(); }
// CHECK-THIRD:        }
// CHECK-THIRD:      }

// CHECK-FOURTH:     namespace NS1 { }
// CHECK-FOURTH:       namespace NS2 {
// CHECK-FOURTH:       namespace NS1 {
// CHECK-FOURTH:         void foo() {}
// CHECK-FOURTH:       }
// CHECK-FOURTH-NOT:   namespace NS3 {
// CHECK-FOURTH:         using NS1::foo;
// CHECK-FOURTH:         void bar() { foo(); }
// CHECK-FOURTH-NOT:   }
// CHECK-FOURTH:     }
