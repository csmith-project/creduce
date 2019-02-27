// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: class A {
class AAA {
// CHECK-NEXT: public:
public:
// CHECK-NEXT: A() {}
  AAA() {}
// CHECK-NEXT: static int m1;
  static int m1;
// CHECK-NEXT: };
};
// CHECK-NEXT: int A::m1;
int AAA::m1;
// CHECK-NEXT: void foo() {
void foo() {
// CHECK-NEXT: A a1;
  AAA a1;
// CHECK-NEXT: A::m1 = 1;
  AAA::m1 = 1;
// CHECK-NEXT: }
}

