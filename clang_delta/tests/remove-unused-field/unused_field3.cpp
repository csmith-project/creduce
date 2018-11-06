// RUN: %clang_delta --transformation=remove-unused-field --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: void foo() {
void foo() {
// CHECK: struct {
  struct {
    int m;
// CHECK: } a[] = {};
  } a[] = {1};
// CHECK: }
}
