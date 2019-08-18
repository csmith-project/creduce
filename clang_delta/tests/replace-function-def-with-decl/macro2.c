// RUN: %clang_delta --transformation=replace-function-def-with-decl --counter=1 --to-counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s

#define M2 {
#define M1 M2
#define M22 }
#define M11 M22
// CHECK: void foo() ; void bar() ;
void foo() {
M11 void bar() M1 M11 
