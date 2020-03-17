// RUN: %clang_delta --transformation=remove-unused-function --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=remove-unused-function --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

// CHECK: class A {
// CHECK-SECOND: class A {
class A {
// CHECK-NOT: ~A()
// CHECK-SECOND: ~A() {}
  ~A() {}
// CHECK: void foo();
// CHECK-SECOND-NOT: void foo()
  void foo();
// CHECK: };
// CHECK-SECOND: };
};

// CHECK: void A::foo() {
// CHECK-SECOND-NOT: void A::foo
void A::foo() {
// CHECK: return;
// CHECK-SECOND-NOT: return;
  return;
// CHECK: }
// CHECK-NOT: }
}
