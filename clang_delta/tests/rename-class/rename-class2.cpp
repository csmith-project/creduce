// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct A {
struct ababababababababab {
// CHECK-NEXT: ~A();
  ~ababababababababab();
// CHECK-NEXT: };
};
// CHECK-NEXT: A::~A() {}
ababababababababab::~ababababababababab() {}

