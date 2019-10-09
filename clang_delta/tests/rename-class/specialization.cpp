// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

template <typename T>
// CHECK: struct A {};
struct AAA {};

// CHECK: template<> struct A<int> {};
template<> struct AAA<int> {};
