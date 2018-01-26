// RUN: %clang_delta --transformation=remove-unused-var --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: void foo() {
void foo() {
// CHECK-NOT: int a;
// CHECK-NEXT: {{ }}
  int a;
// CHECK-NEXT: }
}
