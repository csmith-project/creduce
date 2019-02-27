// RUN: %clang_delta --transformation=local-to-global --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct S *f_s;
// CHECK-NEXT: void f() {
void f() {
// CHECK-NEXT: {{ $}}
  struct S *s;
// CHECK-NEXT: }
}
