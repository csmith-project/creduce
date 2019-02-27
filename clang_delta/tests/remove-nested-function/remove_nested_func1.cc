// RUN: %clang_delta --transformation=remove-nested-function --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

struct S1 {
  S1(int);
};
int foo();
int bar(int);
struct f : S1 {
// CHECK: S1(bar(foo()))
  f() : S1(bar(foo())) {
// CHECK-NEXT: int __trans_tmp_1 = foo();
// CHECK-NEXT: bar(__trans_tmp_1);
    bar(foo());
  }
};
