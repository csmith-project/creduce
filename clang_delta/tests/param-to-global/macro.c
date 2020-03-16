// RUN: %clang_delta --transformation=param-to-global --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

#define TYPE unsigned short int
// CHECK: unsigned short{{.*}} foo_p;
// CHECK-NEXT: void foo(void) {
// CHECK-NEXT: }
void foo(TYPE p) {
}
