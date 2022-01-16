// RUN: %clang_delta --transformation=reduce-array-size --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

int foo() {
// CHECK: int a[5] = {1};
  int a[10] = {1};
// CHECK: return a[4];
  return a[4];
}
