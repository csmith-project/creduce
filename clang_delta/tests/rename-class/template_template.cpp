// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

template<typename>
// CHECK: struct A {};
struct AAA {};

template<template <typename> class >
// CHECK-SECOND: struct A {
struct BBB {
// CHECK-SECOND: A() {}
  BBB() {}
};

// CHECK: BBB<A> b;
// CHECK-SECOND: A<AAA> b;
BBB<AAA> b;
