// RUN: %clang_delta --transformation=remove-unused-field --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct {
struct {
// CHECK-NOT: int x;
// CHECK: {{ }}
  int x;
// CHECK-NEXT: int y;
  int y;
// CHECK-NEXT: int z;
  int z;
// CHECK-NEXT: } s = {
} s = {
// CHECK-NEXT: .y = 2,
  .y = 2,
// CHECK-NEXT: 3,
  3,
// CHECK-NOT .x = 1,
  .x = 1,
};
