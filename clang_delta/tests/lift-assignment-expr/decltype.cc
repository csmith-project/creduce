// RUN: %clang_delta --query-instances=lift-assignment-expr %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: Available transformation instances: 0

template <typename a> auto b() -> decltype(1(a())) {}
