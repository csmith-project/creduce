// RUN: %clang_delta --transformation=empty-struct-to-int --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct S1
struct S1 {
// CHECK: struct {} S;
  struct {} S;
// CHECK-NEXT: int *s1;
  struct S *s1;
};
