// RUN: %clang_delta --transformation=aggregate-to-scalar --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: int a[1], b;
int a[1], b;
// CHECK: a_0;
// CHECK: int foo(void) {
int foo(void) {
// CHECK: return a_0;
  return a[0];
// CHECK: }
}
