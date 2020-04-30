// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

namespace NS1 {
  template <class T> struct Base {};
  template <class T> struct Derived: public Base<T> {
    typename Derived::template Base<double>* p1;
  };
}
template <class T> struct Base {};
template <class T> struct Derived: public Base<T> {
  typename Derived::template Base<double>* p1;
};

// CHECK-NOT:  namespace NS1 {
// CHECK:      template <class T> struct [[BASE:[a-zA-Z_]+NS1_Base]] {};
// CHECK:      template <class T> struct [[DERIVED:[a-zA-Z_]+NS1_Derived]]: public [[BASE]]<T> {
// CHECK:        typename [[DERIVED]]::template [[BASE]]<double>* p1;
// CHECK:      };
// CHECK-NOT:  }
// CHECK:      template <class T> struct Base {};
// CHECK:      template <class T> struct Derived: public Base<T> {
// CHECK:        typename Derived::template [[BASE]]<double>* p1;
// CHECK:      };

