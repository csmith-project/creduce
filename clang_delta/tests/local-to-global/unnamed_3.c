// RUN: %clang_delta --transformation=local-to-global --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: union {
// CHECK-NEXT: int j;
// CHECK-NEXT: } f_l;
// CHECK-NEXT: void f() { 
void f() {
// CHECK-NEXT: long i = ({
  long i = ({
// CHECK-NEXT: {{ $}}
    union {
      int j;
    } l;
// CHECK-NEXT: f_l.j;
    l.j;
// CHECK-NEXT: });
  });
// CHECK-NEXT: }
}
