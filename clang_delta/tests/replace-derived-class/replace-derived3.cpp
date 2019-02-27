// RUN: %clang_delta --transformation=replace-derived-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct a {
struct a {
// CHECK-NEXT: ~a();
  ~a();
// CHECK-NEXT: };
};
// CHECK-NEXT: a *b;
struct abcabcabcabcabcabcabca : a {} *b;
// CHECK-NEXT: void foo() { 
void foo() {
// CHECK-NEXT: b->~a();
   b->~abcabcabcabcabcabcabca();
// CHECK-NEXT: }
}
