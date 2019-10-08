// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

template<typename T>
// CHECK: class A {
class SomeClass3 {
public:
// CHECK: A() {}
  SomeClass3() {}
// CHECK: ~A() {}
  ~SomeClass3() {}
};

template<typename T>
// CHECK-SECOND: class A {
class SomeClass4 {
public:
// CHECK-SECOND: A<T>() {}
  SomeClass4<T>() {}
// CHECK-SECOND: ~A() {}
  ~SomeClass4() {}
};

