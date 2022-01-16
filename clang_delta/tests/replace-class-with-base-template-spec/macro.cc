// RUN: %clang_delta --transformation=replace-class-with-base-template-spec --counter=1  %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: template <typename T>
template <typename T>
// CHECK: struct S {};
struct S {};

// CHECK: #define DEF(NAME) template <class T> struct NAME  {}
#define DEF(NAME) template <class T> struct NAME : S<int> {}
// CHECK: DEF(MyStruct);
DEF(MyStruct);
