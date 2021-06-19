// RUN: %clang_delta --transformation=param-to-local --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

#define X \
  int x = 1;

int foo(int p) {
  X
  return x;
}

// CHECK: #define X \
// CHECK:   int x = 1;

// CHECK: int foo(void) {
// CHECK:   int p = 0;
// CHECK:   X
// CHECK:   return x;
// CHECK: }

