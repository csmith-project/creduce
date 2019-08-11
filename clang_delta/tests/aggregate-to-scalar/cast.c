// RUN: %clang_delta --transformation=aggregate-to-scalar --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

int a1[10];
// CHECK: int a1_1;

void foo() {
  char a;
  int t = 1;
// CHECK: ((char (*)[t]) a)[0][0] = 0;
  ((char (*)[t]) a)[0][0] = 0;
// CHECK: a1_1 = 1;
  a1[1] = 1;
}
