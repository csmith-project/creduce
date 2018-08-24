// RUN: %clang_delta --transformation=reduce-array-dim --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

template <int h>
void foo() {
// CHECK: double k[1];
  double k[1][h];
// CHECK-NEXT: }
}
void bar() {
  foo<1>();
}
