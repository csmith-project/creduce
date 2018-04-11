// RUN: %clang_delta --transformation=remove-unused-field --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct S {
struct S {
// CHECK-NOT: int f1;
// CHECK: {{ }}
  int f1;
// CHECK-NEXT: int f2;
  int f2;
};

void foo() {
// CHECK: S s = { 2};
  S s = {1, 2};
}
