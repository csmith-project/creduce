// RUN: %clang_delta --transformation=reduce-pointer-level --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

struct S {
  int f1;
} g;

void foo() {
  struct S *s = ({
    1 + 2; &g;
  });
}

// CHECK: struct S {
// CHECK:   int f1;
// CHECK: } g;
// CHECK: void foo() {
// CHECK:   struct S s{{ *}};
// CHECK: }

