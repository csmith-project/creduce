// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

// CHECK: struct A {};
struct AAA {};
// CHECK-SECOND: template<typename T> class A {};
template<typename T> class BBB {};
// CHECK: template<> class BBB<int> : public A {};
// CHECK-SECOND: template<> class A<int> : public AAA {};
template<> class BBB<int> : public AAA {};
