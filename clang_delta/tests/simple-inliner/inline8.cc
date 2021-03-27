// RUN: %clang_delta --transformation=simple-inliner --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK-NOT: void foo()
void foo()
{

}

#define MACRO foo()

void bar() {
  MACRO;
}
// CHECK: void bar() {
// CHECK: }
