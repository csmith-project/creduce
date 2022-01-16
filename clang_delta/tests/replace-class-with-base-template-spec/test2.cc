// RUN: %clang_delta --transformation=replace-class-with-base-template-spec --counter=1  %s 2>&1 | %remove_lit_checks | FileCheck %s

template <typename T> class S { };
class A : public S<int> {};
void foo() {
  A a;
  class A b;
}

// CHECK: template <typename T> class S { };
// CHECK: class A  {};
// CHECK: void foo() {
// CHECK:   S<int>  a;
// CHECK:   class S<int>  b;
// CHECK: }
