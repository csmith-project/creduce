// RUN: %clang_delta --transformation=empty-struct-to-int --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct S1
struct S1 {
// CHECK: int S;
  struct {} S;
// CHECK-NEXT: struct S *s1;
  struct S *s1;
};
