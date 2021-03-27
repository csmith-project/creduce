// RUN: %clang_delta --transformation=simple-inliner --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

int printf(const char *format, ...);
void foo(int *p) {
  int *l = p;
  *p = 123;
}
int main() {
  int x = 0;
  int *p = &x;
  foo(p);
  printf("%d\n", *p);
  return 0;
}

// CHECK: int printf(const char *format, ...);
// CHECK-NOT: void foo(int *p)
// CHECK: int main() {
// CHECK:   int x = 0;
// CHECK:   int *p = &x;
// CHECK:   {
// CHECK:   int *[[VAR:.*]] = p;
// CHECK:   {int *p = [[VAR]];
// CHECK:    int *l = p;
// CHECK:    *p = 123;
// CHECK:   }}
// CHECK:   printf("%d\n", *p);
// CHECK:  return 0;
// CHECK: }
