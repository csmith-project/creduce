// RUN: %clang_delta --transformation=local-to-global --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: union {
// CHECK-NEXT: int j;
// CHECK-NEXT: } f_l;
// CHECK-NEXT: void f() { 
void f() {
// CHECK-NEXT: {{ $}}
  union {
    int j;
  } l;
// CHECK-NEXT: long i = 1;
  long i = 1;
// CHECK-NEXT: }
}
