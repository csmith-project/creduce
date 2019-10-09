// RUN: %clang_delta --transformation=replace-derived-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// CHECK: template <int> struct a {};
template <int> struct a {};
// CHECK-NOT: struct b : a<false> {};
template <typename> struct b : a<false> {};
// CHECK: template <template <typename> typename> struct c;
template <template <typename> typename> struct c;
template <typename d> using e = typename d::f;
template <int> struct j;
// CHECK: template <typename g> using h = e<c<j<sizeof(g)>::template i>>;
template <typename g> using h = e<c<j<sizeof(g)>::template i>>;
