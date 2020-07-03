// RUN: %clang_delta --transformation=instantiate-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

template<typename T> class A {
public:       
  template<typename T1> struct C {
    typedef A other;
  };
};
template<typename T1, typename T2> class B {
  typedef typename T2::template C<int>::other type;
};
class B<char, A<char> >;

// CHECK: template<typename T> class A {
// CHECK: public:
// CHECK:  template<typename T1> struct C {
// CHECK:    typedef A other;
// CHECK:  };
// CHECK: };
// CHECK: template<typename T1, typename T2> class B {
// CHECK:   typedef typename A<char>::template C<int>::other type;
// CHECK: };
// CHECK: class B<char, A<char> >;

