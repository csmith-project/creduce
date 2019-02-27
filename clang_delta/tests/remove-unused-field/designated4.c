// RUN: %clang_delta --transformation=remove-unused-field --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct {
struct {
// CHECK-NOT: int z;
// CHECK: {{ }}
  int z;
// CHECK-NEXT: int x;
  int x;
// CHECK-NEXT: int y;
  int y;
// CHECK-NEXT: } s = {
} s = {
// CHECK-NEXT: .y = 2,
  .y = 2,
// CHECK-NOT: 3,
  3,
// CHECK-NEXT .x = 1,
  .x = 1,
};
