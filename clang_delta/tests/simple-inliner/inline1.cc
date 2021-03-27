// RUN: %clang_delta --transformation=simple-inliner --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

#include <iostream>
// CHECK: #include <iostream>
template <typename x0> struct A { static void m_fn1(x0); };
// CHECK: template <typename x0> struct A { static void m_fn1(x0); };
template <typename x0> void A<x0>::m_fn1(x0 p1) { std::cout << p1; }
// CHECK-NOT: A<x0>::m_fn1
int main() { A<char *>::m_fn1("Hello"); }
// CHECK: int main() {{{[[:space:]]}}{char *p1 = "Hello";
// CHECK: std::cout << p1; }
// }
