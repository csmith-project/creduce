// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-namespace --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

namespace NS1 {
  void foo(void) {}
} // NS1
namespace NS2 {
  using NS1::foo;
  void bar() { foo();}
} // NS2
void foo() {}
void func() {foo();}

// CHECK-FIRST-NOT:  namespace NS1 {
// CHECK-FIRST:        void [[FOO:[a-zA-Z_]+NS1_foo]](void) {}
// CHECK-FIRST-NOT:  }
// CHECK-FIRST:      namespace NS2 {
// CHECK-FIRST-NOT:    using NS1::foo
// CHECK-FIRST:        void bar() { [[FOO]]();}
// CHECK-FIRST:      } // NS2
// CHECK-FIRST:      void foo() {}
// CHECK-FIRST:      void func() {foo();}

// CHECK-SECOND:     namespace NS1 {
// CHECK-SECOND:       void foo(void) {}
// CHECK-SECOND:     } // NS1
// CHECK-SECOND-NOT: namespace NS2 {
// CHECK-SECOND-NOT:   using NS1::foo
// CHECK-SECOND:       void bar() { NS1::foo();}
// CHECK-SECOND-NOT: }
// CHECK-SECOND:     void foo() {}
// CHECK-SECOND:     void func() {foo();}

