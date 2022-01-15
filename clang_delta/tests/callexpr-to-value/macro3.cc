// RUN: %clang_delta --transformation=callexpr-to-value --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: bool baz();
bool baz();
// CHECK: #define MACRO1(expr) expr
#define MACRO1(expr) expr
// CHECK: #define MACRO2 MACRO1(baz())
#define MACRO2 MACRO1(baz())
// CHECK: void foo() {
void foo() {
// CHECK: 0;
  MACRO2;
// CHECK: }
}
