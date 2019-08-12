// RUN: %clang_delta --transformation=callexpr-to-value --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

#define macro bar(1,2)
int bar(int p1, int p2) {
  return p1 + p2;
}

void foo(void) {
// CHECK: int x = 0;
  int x = macro;
}
