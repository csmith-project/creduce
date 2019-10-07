// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

template<typename T>
// CHECK: class A {
class SomeClass1 {
public:
// CHECK: A() {}
  SomeClass1() {}
// CHECK: ~A<T>() {} 
  ~SomeClass1<T>() {}
};

template<typename T>
// CHECK-SECOND: class A {
class SomeClass2 {
public:
// CHECK-SECOND: A<T>() {}
  SomeClass2<T>() {}
// CHECK-SECOND: ~A<T>() {}
  ~SomeClass2<T>() {}
};
