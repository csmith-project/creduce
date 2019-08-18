// RUN: %clang_delta --transformation=callexpr-to-value --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: #define b(a) a
#define b(a) a
// CHECK-NEXT: c() {
c() {
// CHECK-NEXT: 0;
  d b((d));
// CHECK-NEXT: }
}
