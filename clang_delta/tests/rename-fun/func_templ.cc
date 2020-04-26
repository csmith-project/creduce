// RUN: %clang_delta --transformation=rename-fun --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

template <typename> struct S1 {
  typedef S1 b;
  void operator<<(b ());
};

// CHECK: template <typename T> S1<T> fn1();
template <typename T> S1<T> fun();
S1<char> s;

// CHECK: int fn2() {
int foo() {
// CHECK: s << fn1;
  s << fun;
}
