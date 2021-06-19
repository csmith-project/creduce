// RUN: %clang_delta --transformation=param-to-local --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

int foo(int x) {
  int y = 1;
  return x + y;
}

// CHECK: int foo(void) {
// CHECK:   int x = 0;
// CHECK:   int y = 1;
// CHECK:   return x + y;
// CHECK: }
