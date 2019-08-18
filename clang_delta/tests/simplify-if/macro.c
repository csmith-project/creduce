// RUN: %clang_delta --transformation=simplify-if --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

#define LBRAC {
#define RBRAC }

// CHECK: void foo(void) {
void foo(void) {
// CHECK: !0;
// CHECK: LBRAC
   if (!0) LBRAC
// CHECK: 0
     0;
// CHECK: RBRAC
   RBRAC
}
