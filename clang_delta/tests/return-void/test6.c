// RUN: %clang_delta --transformation=return-void --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

#define M(x) (x)
// CHECK: void test(void) {
const void *test(void) {
  int *t = 0;
// CHECK-NOT: return
  return M(t);
}
