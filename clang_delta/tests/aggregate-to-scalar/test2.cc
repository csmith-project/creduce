// RUN: %clang_delta --transformation=aggregate-to-scalar --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=aggregate-to-scalar --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

struct S {
  int **f1;
};

void foo() {
  int *a[16];
  struct S s = { a };
// CHECK: int *s_0_0;
// CHECK-NEXT: s_0_0 = 0;
// CHECK-SECOND: int **s_0 = a;
// CHECK-SECOND-NEXT: s_0[0] = 0;
  s.f1[0] = 0;
}
