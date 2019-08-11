// RUN: %clang_delta --transformation=aggregate-to-scalar --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

struct S {
  int f1;
} a, b[1];
// CHECK: int a_0;

int foo()
{
// CHECK: return a_0 + b[0].f1;
  return a.f1 + b[0].f1;
}
