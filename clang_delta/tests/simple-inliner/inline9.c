// RUN: %clang_delta --transformation=simple-inliner --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK-NOT: int c(int a) __attribute__((alias
int c(int a) __attribute__((alias("foo")));
void d() { c(1); }
int foo(int a) {
  return a;
}

// CHECK: void d() {
// CHECK: int [[VAR:.*]]; {int a = 1;
// CHECK: [[VAR]] =  a;
// CHECK: }
// CHECK: [[VAR]]; }
// CHECK: int foo(int a) {
// CHECK:   return a;
// CHECK: }
