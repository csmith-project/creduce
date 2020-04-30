// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-namespace --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

namespace NS1 {
template <int> struct foo { operator long double() const; };
namespace NS2 {
typedef struct {
  int f;
} S;
S bar(int);
}
using namespace NS2;
template <> foo<32>::operator long double() const {
  long double t;
  *reinterpret_cast<S *>(&t) = bar(0);
  return t;
}
}

// CHECK-FIRST-NOT: namespace NS1 {
// CHECK-FIRST:     template <int> struct foo { operator long double() const; };
// CHECK-FIRST:     namespace NS2 {
// CHECK-FIRST:     typedef struct {
// CHECK-FIRST:       int f;
// CHECK-FIRST:     } S;
// CHECK-FIRST:     S bar(int);
// CHECK-FIRST:     }
// CHECK-FIRST-NOT: using namespace NS2;
// CHECK-FIRST:     template <> foo<32>::operator long double() const {
// CHECK-FIRST:       long double t;
// CHECK-FIRST:       *reinterpret_cast<NS2::S *>(&t) = NS2::bar(0);
// CHECK-FIRST:       return t;
// CHECK-FIRST:     }
// CHECK-FIRST-NOT: }

// CHECK-SECOND:     namespace NS1 {
// CHECK-SECOND:     template <int> struct foo { operator long double() const; };
// CHECK-SECOND-NOT: namespace NS2 {
// CHECK-SECOND:     typedef struct {
// CHECK-SECOND:       int f;
// CHECK-SECOND:     } S;
// CHECK-SECOND:     S bar(int);
// CHECK-SECOND-NOT: }
// CHECK-SECOND-NOT: using namespace NS2;
// CHECK-SECOND:     template <> foo<32>::operator long double() const {
// CHECK-SECOND:       long double t;
// CHECK-SECOND:       *reinterpret_cast<S *>(&t) = bar(0);
// CHECK-SECOND:       return t;
// CHECK-SECOND:     }
// CHECK-SECOND: }

