// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

template<template<class> class BBB>
// CHECK: struct A {
struct AAA {
// CHECK-SECOND: template <class T> struct A {
  template <class T> struct CCC {
// CHECK: static BBB<T> a;
// CHECK-SECOND: static BBB<T> a;
      static BBB<T> a;
  };
};
