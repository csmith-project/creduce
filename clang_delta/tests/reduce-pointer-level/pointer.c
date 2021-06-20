// RUN: %clang_delta --transformation=reduce-pointer-level --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

long *x;
void foo() {
  int i;
  for (i = 0; i < 10; i++) {
    x[i] += 1;
    x[i] = 2;
  }
}

// CHECK: long x;
// CHECK: void foo() {
// CHECK:   int i;
// CHECK:   for (i = 0; i < 10; i++) {
// CHECK:     x += 1;
// CHECK:     x = 2;
// CHECK:   }
// CHECK: }

