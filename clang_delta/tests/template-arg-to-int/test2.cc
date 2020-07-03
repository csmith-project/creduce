// RUN: %clang_delta --transformation=template-arg-to-int --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct S {};
struct S {};
// CHECK: template <typename T> void foo(void) { }
template <typename T> void foo(void) { }
// CHECK: template<> void foo<int>(void) { }
template<> void foo<S>(void) { }
