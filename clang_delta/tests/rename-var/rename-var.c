// RUN: %clang_delta --transformation=rename-var --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

struct S {
// CHECK: int *f1;
  int *f1;
};
// CHECK: int a;
int abcdef;
// CHECK-NEXT: struct S b = {&a};
struct S b = {&abcdef};
