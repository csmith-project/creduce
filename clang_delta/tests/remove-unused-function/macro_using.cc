// RUN: %clang_delta --transformation=remove-unused-function --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: #define USING using ::foo;
#define USING using ::foo;
// CHECK-NOT: void foo
void foo();
// CHECK-NOT: USING 
USING
