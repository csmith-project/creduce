// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

namespace NS1 {
  void foo();
}
namespace cl = NS1;
void bar() {cl::foo(); }

// CHECK-NOT: namespace NS1 {
// CHECK:       void foo();
// CHECK-NOT: }
// CHECK-NOT: cl;
// CHECK: void bar() {foo(); }
