// RUN: %clang_delta --transformation=remove-unused-function --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-1
// RUN: %clang_delta --transformation=remove-unused-function --counter=1 --to-counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-1-3

void *foo(int p1, int p2), bar(), *baz();
// CHECK-1-NOT: void *foo

// CHECK-1-3-NOT: void {{.*}};

