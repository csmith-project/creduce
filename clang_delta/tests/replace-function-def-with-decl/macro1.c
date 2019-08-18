// RUN: %clang_delta --transformation=replace-function-def-with-decl --counter=1 --to-counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s

#define M2 {
#define M1 M2
#define M22 }
#define M11 M22
// CHECK: int foo() ;
int foo() M1
  return 0;
}
// CHECK-NEXT: void bar() ;
void bar() {
  int x = 0;
M11
