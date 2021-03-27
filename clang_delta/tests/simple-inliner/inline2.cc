// RUN: %clang_delta --transformation=simple-inliner --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK-NOT: bar() {
int bar() {
  int y = 0;
  return y;
}
void foo() {
  int x = bar();
}

// CHECK: void foo() {
// CHECK: int [[VAR:.*]];
// CHECK: int y = 0;
// CHECK: [[VAR]] =  y;
// CHECK: int x = [[VAR]];
// CHECK: }
