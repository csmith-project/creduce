// RUN: %clang_delta --transformation=replace-class-with-base-template-spec --counter=1  %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-1
// RUN: %clang_delta --transformation=replace-class-with-base-template-spec --counter=3  %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-3
// RUN: %clang_delta --transformation=replace-class-with-base-template-spec --counter=6  %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-6

template < typename > struct A;
template < typename T1 > struct B {
  typedef T1 first;
};
struct C {
  template < typename T1, typename T2 > struct D {
    typedef typename T1::first base;
    class node:base {
      typedef typename A < T2 >::type T;
      typedef T pT;
      typedef pT param_T;
    public:  base::initialize;
      static int initialize (param_T);
    };
  };
};
class F { public:void initialize (); };
template < typename T > struct G {
  typedef T item;
};
template < typename > struct H;
template < typename Node > struct A <H < Node > > {
  typedef typename Node::item type;
};
struct L:G < int > { };
struct M:G < int > { };
struct N:G < int > { };
struct O:G < int > { };
struct P:G < int > { };
struct Q:G < int > { };
class I;
class J {
  struct R:C::D < B < C::D < B < C::D < B < C::D < B < C::D < B < C::D < B <
    C::D < B < C::D < B < F >, H < G < int > > >::node >,
    H < Q > >::node >, H < P > >::node >,
    H < O > >::node >, H < G < I > > >::node >,
    H < N > >::node >, H < M > >::node >,
    H < L > >::node {
  };
  template < typename T > void foo (T &, int) {
    R::initialize (0);
  }
public:  template < typename T > J (const T & p1) {
    foo (p1, 0);
  }
};

class I {
  class K {
    K (int[]):m (0) { }
    J m;
  };
};

// CHECK-1: struct L { };
// CHECK-1: struct M:G < int > { };
// CHECK-1: struct N:G < int > { };
// CHECK-1: struct O:G < int > { };
// CHECK-1: struct P:G < int > { };
// CHECK-1: struct Q:G < int > { };
// CHECK-1: class I;
// CHECK-1: class J {
// CHECK-1:   struct R:C::D < B < C::D < B < C::D < B < C::D < B < C::D < B < C::D < B <
// CHECK-1:     C::D < B < C::D < B < F >, H < G < int > > >::node >,
// CHECK-1:     H < Q > >::node >, H < P > >::node >,
// CHECK-1:     H < O > >::node >, H < G < I > > >::node >,
// CHECK-1:     H < N > >::node >, H < M > >::node >,
// CHECK-1:     H < G<int>  > >::node {
// CHECK-1:   };
 
// CHECK-3: struct N { };
// CHECK-3: struct O:G < int > { };
// CHECK-3: struct P:G < int > { };
// CHECK-3: struct Q:G < int > { };
// CHECK-3: class I;
// CHECK-3: class J {
// CHECK-3:   struct R:C::D < B < C::D < B < C::D < B < C::D < B < C::D < B < C::D < B <
// CHECK-3:     C::D < B < C::D < B < F >, H < G < int > > >::node >,
// CHECK-3:     H < Q > >::node >, H < P > >::node >,
// CHECK-3:     H < O > >::node >, H < G < I > > >::node >,
// CHECK-3:     H < G<int>  > >::node >, H < M > >::node >,
// CHECK-3:     H < L > >::node {

// CHECK-6: struct P:G < int > { };
// CHECK-6: struct Q { };
// CHECK-6: class I;
// CHECK-6: class J {
// CHECK-6:   struct R:C::D < B < C::D < B < C::D < B < C::D < B < C::D < B < C::D < B <
// CHECK-6:     C::D < B < C::D < B < F >, H < G < int > > >::node >,
// CHECK-6:     H < G<int>  > >::node >, H < P > >::node >,
// CHECK-6:     H < O > >::node >, H < G < I > > >::node >,
// CHECK-6:     H < N > >::node >, H < M > >::node >,
// CHECK-6:     H < L > >::node {
// CHECK-6:   };
