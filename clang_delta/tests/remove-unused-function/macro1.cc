// RUN: %clang_delta --transformation=remove-unused-function --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: #define EXTERN extern
#define EXTERN extern
// CHECK-NOT: EXTERN int foo();
EXTERN int foo();
