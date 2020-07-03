// RUN: %clang_delta --transformation=instantiate-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: template <typename T1, typename T2 = int> struct A {
template <typename T1, typename T2 = int> struct A {
// CHECK: void bar(int p);
  void bar(T2 p);
// CHECK: };
};
// CHECK: void foo() {
void foo() {
// CHECK: A<int> a;
  A<int> a;
// CHECK: }
}
