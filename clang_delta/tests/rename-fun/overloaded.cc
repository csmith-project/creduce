// RUN: %clang_delta --transformation=rename-fun --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
template < class, class, class > class a {};
template < class i, class az, class ba, class bb, class bc >
// CHECK: int operator*(a< i, az, bb >, a< i, ba, bc >);
int operator*(a< i, az, bb >, a< i, ba, bc >);
// CHECK: void fn1() {
void bh() {
  a<int, int, int> bg;
// CHECK: bg * bg
  bg * bg;
}
