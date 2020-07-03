// RUN: %clang_delta --query-instances=instantiate-template-param %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: Available transformation instances: 0

template <typename T> struct S {
  T bar(T p) { return p; }
};
class T {};
void foo() {
  struct S<T> s;
}
