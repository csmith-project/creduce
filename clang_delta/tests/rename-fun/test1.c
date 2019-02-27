// RUN: %clang_delta --transformation=rename-fun --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

#include "test1.h"

// CHECK: void fn1();
void bar();
// CHECK: int foo()
int foo() { return 0; }
