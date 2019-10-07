// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: class A {
class AAA {
public:
// CHECK: A() {}
  AAA() {}
  static int m1;
};

// CHECK: int A::m1;
int AAA::m1;

int foo(void) {
// CHECK: A a1;
  AAA a1;
// CHECK: A::m1 = 1;
  AAA::m1 = 1;
  return 0;
}
