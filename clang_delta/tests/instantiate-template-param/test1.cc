// RUN: %clang_delta --transformation=instantiate-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: template <typename T> struct S {
template <typename T> struct S {
// CHECK: int bar(int p) { return p; }
  T bar(T p) { return p; }
// CHECK: };
};
// CHECK: void foo() {
void foo() {
// CHECK:  struct S<int> s;
  struct S<int> s;
// CHECK: }
}
