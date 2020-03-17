// RUN: %clang_delta --transformation=remove-unused-function --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: template <typename T> struct S {;};
template <typename T> struct S {template <typename T1> void foo();};
// CHECK-NOT: void S<T>::foo()
template<typename T> template<typename T1> void S<T>::foo() { }
