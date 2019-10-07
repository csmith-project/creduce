// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=rename-class --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=rename-class --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD
// RUN: %clang_delta --transformation=rename-class --counter=4 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FOURTH

namespace NS1 {
// CHECK-THIRD: template<class T> class A;
// CHECK-FOURTH: template<class T> class Class1;
  template<class T> class Class1;
// CHECK: template<class T> class A {};
// CHECK-SECOND: template<class T> class Basic {};
  template<class T> class Basic {};
// CHECK: template<class T> class Class1: public A<T> {};
// CHECK-SECOND: template<class T> class Class1: public Basic<T> {};
// CHECK-THIRD: template<class T> class A: public Basic<T> {};
// CHECK-FOURTH: template<class T> class Class1: public Basic<T> {};
  template<class T> class Class1: public Basic<T> {};
}
namespace NS2 {
// CHECK-THIRD: class Class1;
// CHECK-FOURTH: class A;
  class Class1;
// CHECK: class Basic {};
// CHECK-SECOND: class A {};
  class Basic {};
// CHECK: class Class1: public Basic {};
// CHECK-SECOND: class Class1: public A {};
// CHECK-THIRD: class Class1: public Basic {};
// CHECK-FOURTH: class A: public Basic {};
  class Class1: public Basic {};
}
