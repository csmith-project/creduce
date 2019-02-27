// RUN: %clang_delta --transformation=replace-derived-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

struct a {
  ~a();
};
// CHECK-NOT: struct X
struct X : a {};
// CHECK: foo() {
void foo() {
// CHECK-NEXT: a *b;
   X *b;
// CHECK-NEXT: b->~a();
   b->~X();
// CHECK-NEXT: }
}
