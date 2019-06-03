// RUN: %clang_delta --transformation=copy-propagation --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct S {
struct S {
  S() {
    int t1; int a;
    int b = a;
    {
      m;
// CHECK: int a = a;
      int a = b;
// CHECK: t1 = a;
      t1 = a;
    }
  }
  int m;
};
