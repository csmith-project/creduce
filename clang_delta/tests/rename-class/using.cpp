// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=rename-class --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD
// RUN: %clang_delta --transformation=rename-class --counter=4 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FOURTH

// CHECK: template <class T> struct A {
template <class T> struct Base {
  typedef T value_type;
};

// CHECK: template <class T> struct Derived : A<T> {
// CHECK-FOURTH: template <class T> struct A : Base<T> {
template <class T> struct Derived : Base<T> {
// CHECK: using A<T>::value_type;
  using Base<T>::value_type;
// CHECK: typename A<T>::value_type get();
  typename Base<T>::value_type get();
};

// CHECK-SECOND: template<typename T> struct A {
template<typename T> struct SomeClass {
// CHECK-SECOND: A() {}
  SomeClass() {}
// CHECK-SECOND: ~A() {}
  ~SomeClass() {}
};

// CHECK-THIRD: template<typename T> struct A {
template<typename T> struct MyTypeDef {
// CHECK-SECOND: typedef A<T> type;
  typedef SomeClass<T> type;
};

template<typename T>
// CHECK-SECOND: using MyType = A<T>;
using MyType = SomeClass<T>;
MyType<int> mytype;

template <class T> using Ptr = T*;
Ptr<int> ip;
