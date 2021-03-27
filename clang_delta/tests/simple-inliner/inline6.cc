// RUN: %clang_delta --transformation=simple-inliner --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: int printf(const char *format, ...);
int printf(const char *format, ...);
// CHECK-NOT: foo(int p)
void foo(int p) {
  int *l = &p;
  *l = 123;
}
int main() {
  int p = 0;
  foo(p);
  printf("%d\n", p);
  return 0;
}

// CHECK: int main() {
// CHECK: int p = 0;
// CHECK: {
// CHECK:  int [[VAR:.*]] = p;
// CHECK: {int p = [[VAR]];
// CHECK:  int *l = &p;
// CHECK:  *l = 123;
// CHECK: }}
// CHECK:  printf("%d\n", p);
// CHECK:  return 0;
// CHECK: }
