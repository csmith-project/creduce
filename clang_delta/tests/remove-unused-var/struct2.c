// RUN: %clang_delta --transformation=remove-unused-var --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=remove-unused-var --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

// CHECK: void foo() {
// CHECK-SECOND: void foo() {
void foo() {
// CHECK-NEXT: struct S { } s2;
// CHECK-SECOND-NEXT: struct S { } s1;
  struct S { } s1, s2;
// CHECK-NEXT: }
// CHECK-SECOND-NEXT: }
}
