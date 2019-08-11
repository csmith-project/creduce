// RUN: %clang_delta --transformation=aggregate-to-scalar --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

struct S {
  int a[2];
};

int foo() {
  struct S s = {1,2};
// CHECK: int s_0_0 = 1;
// CHECK: return s_0_0;
  return s.a[0];
}
