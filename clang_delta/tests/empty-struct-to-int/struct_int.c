// RUN: %clang_delta --transformation=empty-struct-to-int --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct {
struct {
// CHECK-NEXT: int f1;
  int f1;
// CHECK-NEXT: int f3[];
  struct {
    int f2;
  } f3[];
// CHECK-NEXT: } s1 = {};
} s1 = {};
