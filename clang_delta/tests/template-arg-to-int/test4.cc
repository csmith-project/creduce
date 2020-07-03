// RUN: %clang_delta --transformation=template-arg-to-int --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

template <typename T1> struct pair {
  typedef T1 first;
};
struct S1 {
  void foo() {}
};
struct S2 {
  typedef pair<S1>::first type;
};

// CHECK: template <typename T1> struct pair {
// CHECK:   typedef T1 first;
// CHECK: };
// CHECK: struct S1 {
// CHECK:   void foo() {}
// CHECK: };
// CHECK: struct S2 {
// CHECK:   typedef pair<int>::first type;
// CHECK: };
