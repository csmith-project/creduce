// RUN: %clang_delta --transformation=remove-unused-function --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=remove-unused-function --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=remove-unused-function --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD
// RUN: %clang_delta --transformation=remove-unused-function --counter=4 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FOURTH

// CHECK-NOT: void foo();
// CHECK-SECOND: void foo();
// CHECK-THIRD: void foo();
// CHECK-FOURTH: void foo();
void foo();
// CHECK: void bar(int x) {
// CHECK-SECOND-NOT: void bar(int x) {
// CHECK-THIRD: void bar(int x) {
// CHECK-FOURTH: void bar(int x) {
void bar(int x) {

// CHECK: }
// CHECK-SECOND-NOT: }
// CHECK-THIRD: }
// CHECK-FOURTH: }
}
// CHECK: extern int baz(int x,
// CHECK-SECOND: extern int baz(int x,
// CHECK-THIRD-NOT: extern int baz(int x,
// CHECK-FOURTH: extern int baz(int x,
extern int baz(int x,
// CHECK: int y);
// CHECK-SECOND: int y);
// CHECK-THIRD-NOT: int y);
// CHECK-FOURTH: int y);
           int y);

// CHECK: extern "C" {
// CHECK-SECOND: extern "C" {
// CHECK-THIRD: extern "C" {
// CHECK-FOURTH: extern "C" {
extern "C" {
// CHECK: long long f1() {}
// CHECK-SECOND: long long f1() {}
// CHECK-THIRD: long long f1() {}
// CHECK-FOURTH-NOT: long long f1() {}
  long long f1() {}
// CHECK: }
// CHECK-SECOND: }
// CHECK-THIRD: }
// CHECK-FOURTH: }
}
