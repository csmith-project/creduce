// RUN: %clang_delta --transformation=reduce-class-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

template<typename T1, typename T2> class A { };
class B : public A<int, int> {
  int m;
  B(int x) : m(x) {}
};

// CHECK-FIRST: template< typename T2> class A { };
// CHECK-FIRST: class B : public A< int> {
// CHECK-FIRST:   int m;
// CHECK-FIRST:   B(int x) : m(x) {}
// CHECK-FIRST: };

// CHECK-SECOND: template<typename T1> class A { };
// CHECK-SECOND: class B : public A<int> {
// CHECK-SECOND:   int m;
// CHECK-SECOND:   B(int x) : m(x) {}
// CHECK-SECOND: };

