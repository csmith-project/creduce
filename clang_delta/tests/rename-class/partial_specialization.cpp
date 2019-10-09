// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

template <typename T, int N>
// CHECK: struct A {
struct S1 {
  T value() const { return N; }
};

template <typename T>
// CHECK: struct A <T, 3> {
struct S1 <T, 3> {
  T value() const { return 0; }
};

