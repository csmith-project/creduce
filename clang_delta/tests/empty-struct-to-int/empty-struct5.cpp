// RUN: %clang_delta --transformation=empty-struct-to-int --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK-NOT: struct S2
struct S2 {
// CHECK-NOT: int a[2]
  char a[2];
// CHECK-NOT: };
};
// CHECK: int s
S2 s = {{1,2}};
