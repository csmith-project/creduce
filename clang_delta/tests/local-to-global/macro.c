// RUN: %clang_delta --transformation=local-to-global --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

#define TYPE unsigned short int
// CHECK: unsigned short{{.*}} foo_t;
// CHECK-NEXT: void foo() {
void foo() {
// CHECK-NEXT:  {{^ +$}}
  TYPE t;
// CHECK-NEXT: }
}
