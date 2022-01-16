// RUN: %clang_delta --transformation=reduce-class-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

// CHECK-FIRST: #define DEF(NAME) template < class = void> struct NAME {};
// CHECK-SECOND: #define DEF(NAME) template <class _Tp> struct NAME {};
#define DEF(NAME) template <class _Tp, class = void> struct NAME {};
// CHECK-FIRST: DEF(MyStruct1);
// CHECK-SECOND: DEF(MyStruct1);
DEF(MyStruct1);
// CHECK-SECOND: DEF(S);
DEF(S);
// CHECK-SECOND: DEF(MyS2);
DEF(MyS2);

