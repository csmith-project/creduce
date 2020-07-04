// RUN: %clang_delta --transformation=reduce-class-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

template<typename T1, typename T2> struct B;
template<typename T1, typename T2>
struct A {
  typedef T1 type;
};
template<typename T1, typename T2>
struct B {
  typedef typename A<T1, T2>::type type;
};

// CHECK: template<typename T1, typename T2> struct B;
// CHECK: template<typename T1>
// CHECK: struct A {
// CHECK:   typedef T1 type;
// CHECK: };
// CHECK: template<typename T1, typename T2>
// CHECK: struct B {
// CHECK:   typedef typename A<T1>::type type;
// CHEKC: };

