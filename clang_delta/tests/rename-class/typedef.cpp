// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

// CHECK: template <class T> class A {};
template <class T> class XYZ {};

// CHECK: typedef A<int> BBB;
typedef XYZ<int> BBB;

// CHECK-SECOND: class A : BBB {};
class CCC : BBB {};
