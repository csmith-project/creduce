// RUN: %clang_delta --transformation=remove-unused-field --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct {
struct {
// CHECK-NEXT: int x;
  int x;
// CHECK-NOT: int y;
// CHECK: {{ }}
  int y;
// CHECK-NEXT: int z;
  int z;
// CHECK-NEXT: } s = {
} s = {
// CHECK-NOT: .y = 2,
// CHECK-NEXT: {{ }}
  .y = 2,
// CHECK-NEXT: 3,
  3,
// CHECK-NEXT .x = 1,
  .x = 1,
};
