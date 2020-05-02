// RUN: %clang_delta --transformation=remove-unused-function --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-unused-function --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

struct S1 {
  S1() = default;
};
// CHECK-FIRST: struct S1 {
// CHECK-FIRST: {{^ *$}}
// CHECK-FIRST: };

template <typename T>
struct S2 {
  S2() = default;
};
// CHECK-SECOND: struct S2 {
// CHECK-SECOND: {{^ *$}}
// CHECK-SECOND: };
