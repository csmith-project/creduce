// RUN: %clang_delta --transformation=aggregate-to-scalar --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

struct S {
 int f1;
 int f2;
};
struct S s = {
  .f1 = 0,
  .f2 = 16
};
// CHECK: int s_0 = 0;

void foo() {
// CHECK: s_0++;
  s.f1++;
}
