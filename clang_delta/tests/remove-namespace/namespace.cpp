// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-namespace --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=remove-namespace --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD

namespace NS1 {
namespace NS2 {
  template<class T>
  T foo(T p1, T p2);

  template<class T>
  T operator+(T p1, T p2);
  int func();
}
}
namespace NS3 {
  struct S {};
  S s1, s2;
  using namespace NS1::NS2;
  int bar() {
    func();
    return 0;
  }
}

// CHECK-FIRST-NOT: namespace NS1
// CHECK-FIRST:     namespace NS2 {
// CHECK-FIRST:       template<class T>
// CHECK-FIRST:       T foo(T p1, T p2);
// CHECK-FIRST:       template<class T>
// CHECK-FIRST:       T operator+(T p1, T p2);
// CHECK-FIRST:       int func();
// CHECK-FIRST:     }
// CHECK-FIRST-NOT: }
// CHECK-FIRST:     namespace NS3 {
// CHECK-FIRST:       struct S {};
// CHECK-FIRST:       S s1, s2;
// CHECK-FIRST:       using namespace NS2;
// CHECK-FIRST:       int bar() {
// CHECK-FIRST:         func();
// CHECK-FIRST:         return 0;
// CHECK-FIRST:       }
// CHECK-FIRST:     }

// CHECK-SECOND:     namespace NS1
// CHECK-SECOND-NOT: namespace NS2 {
// CHECK-SECOND:       template<class T>
// CHECK-SECOND:       T foo(T p1, T p2);
// CHECK-SECOND:       template<class T>
// CHECK-SECOND:       T operator+(T p1, T p2);
// CHECK-SECOND:       int func();
// CHECK-SECOND-NOT: }
// CHECK-SECOND:     }
// CHECK-SECOND:     namespace NS3 {
// CHECK-SECOND:       struct S {};
// CHECK-SECOND:       S s1, s2;
// CHECK-SECOND:       using namespace NS1;
// CHECK-SECOND:       int bar() {
// CHECK-SECOND:         func();
// CHECK-SECOND:         return 0;
// CHECK-SECOND:       }
// CHECK-SECOND:     }

// CHECK-THIRD:     namespace NS1
// CHECK-THIRD:     namespace NS2 {
// CHECK-THIRD:       template<class T>
// CHECK-THIRD:       T foo(T p1, T p2);
// CHECK-THIRD:       template<class T>
// CHECK-THIRD:       T operator+(T p1, T p2);
// CHECK-THIRD:       int func();
// CHECK-THIRD:     }
// CHECK-THIRD:     }
// CHECK-THIRD-NOT: namespace NS3 {
// CHECK-THIRD:       struct S {};
// CHECK-THIRD:       S s1, s2;
// CHECK-THIRD-NOT:   using namespace
// CHECK-THIRD:       int bar() {
// CHECK-THIRD:         NS1::NS2::func();
// CHECK-THIRD:         return 0;
// CHECK-THIRD:       }
// CHECK-THIRD-NOT: }

