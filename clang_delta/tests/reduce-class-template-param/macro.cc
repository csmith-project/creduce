// RUN: %clang_delta --transformation=reduce-class-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: #define DEF(NAME) template < class = void> struct NAME {};
#define DEF(NAME) template <class _Tp, class = void> struct NAME {};
// CHECK: DEF(S);
DEF(S);
