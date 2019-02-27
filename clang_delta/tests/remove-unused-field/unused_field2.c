// RUN: %clang_delta --transformation=remove-unused-field --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct S {
struct S {
// CHECK: int f1;
  int f1;
// CHECK-NOT: int f2;
// CHECK: {{ }}
  int f2;
};

void foo() {
// CHECK: S s = {1};
  S s = {1, 2};
}
