// RUN: %clang_delta --transformation=simplify-recursive-template-instantiation --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: template <typename T>
template <typename T>
// CHECK: struct array {};
struct array {};

// CHECK: array<int> x;
array<array<int>> x;
