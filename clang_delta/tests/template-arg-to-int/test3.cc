// RUN: %clang_delta --transformation=template-arg-to-int --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

struct S1 { void bar() {} };
template <typename T1> struct pair {
  T1 s;
  void foo() { s.bar(); }
};
pair<S1> v;

// CHECK: struct S1 { void bar() {} };
// CHECK: template <typename T1> struct pair {
// CHECK:   T1 s;
// CHECK:   void foo() { s.bar(); }
// CHECK: };
// CHECK: pair<int> v;
