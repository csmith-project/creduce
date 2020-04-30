// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK

void foo();
namespace NS2 {
  using ::foo;
  void bar () { foo(); }
}

// CHECK:     void foo();
// CHECK-NOT: namespace NS2 {
// CHECK-NOT:   using ::foo;
// CHECK:       void bar () { foo(); }
// CHECK-NOT: }

