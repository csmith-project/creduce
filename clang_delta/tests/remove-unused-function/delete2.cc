// RUN: %clang_delta --transformation=remove-unused-function --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-unused-function --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=remove-unused-function --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD
// RUN: %clang_delta --transformation=remove-unused-function --counter=4 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FOURTH

struct S1 {
  S1 foo() = delete;
};
// CHECK-FIRST: struct S1 {
// CHECK-FIRST: {{^ *$}}
// CHECK-FIRST: };

struct S2 {
  template <typename T>
  S2 foo() = delete;
};
// CHECK-SECOND: struct S2 {
// CHECK-SECOND: {{^ *; *$}}
// CHECK-SECOND: };

template <typename T>
struct S3 {
  S3 foo() = delete;
};
// CHECK-THIRD: struct S3 {
// CHECK-THIRD: {{^ *$}}
// CHECK-THIRD: };

template <typename T>
struct S4 {
  S4 foo();
};
// CHECK-FOURTH: struct S4 {
// CHECK-FOURTH: {{^ *$}}
// CHECK-FOURTH: };
