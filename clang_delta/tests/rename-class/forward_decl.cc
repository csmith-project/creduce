// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=rename-class --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD

namespace NS1 {
// CHECK: template<class T, class T2 = T> struct A;
  template<class T, class T2 = T> struct S1;
// CHECK-SECOND: template<class T> struct A {};
  template<class T> struct S2 {};
}

namespace NS2 {
  template<class T, class T2 = T, class T3 = void>
// CHECK: struct S1 : NS1::A<typename NS1::S2<T>::type, T2> {};
// CHECK-SECOND: struct S1 : NS1::S1<typename NS1::A<T>::type, T2> {};
// CHECK-THIRD: struct A : NS1::S1<typename NS1::S2<T>::type, T2> {};
  struct S1 : NS1::S1<typename NS1::S2<T>::type, T2> {};
}

namespace NS1 {
// CHECK: template<class T, class T2> struct A {};
// CHECK-SECOND: template<class T, class T2> struct S1 {};
  template<class T, class T2> struct S1 {};
}
