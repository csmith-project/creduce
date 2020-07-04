// RUN: %clang_delta --transformation=reduce-class-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=4 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FOURTH

template<bool  , typename>
struct AAA { };
template<typename T>
struct AAA<true, T *>
{ };
template<typename, typename>
struct BBB {};
template<typename T>
struct BBB<T &, struct AAA<true, unsigned int> > {};

// CHECK-FIRST: template< typename>
// CHECK-FIRST: struct AAA { };
// CHECK-FIRST: template<typename T>
// CHECK-FIRST: struct AAA< T *>
// CHECK-FIRST: { };
// CHECK-FIRST: template<typename, typename>
// CHECK-FIRST: struct BBB {};
// CHECK-FIRST: template<typename T>
// CHECK-FIRST: struct BBB<T &, struct AAA< unsigned int> > {};

// CHECK-SECOND: template<bool  >
// CHECK-SECOND: struct AAA { };
// CHECK-SECOND: template<>
// CHECK-SECOND: struct AAA<true>
// CHECK-SECOND: { };
// CHECK-SECOND: template<typename, typename>
// CHECK-SECOND: struct BBB {};
// CHECK-SECOND: template<typename T>
// CHECK-SECOND: struct BBB<T &, struct AAA<true> > {};

// CHECK-THIRD: template<bool  , typename>
// CHECK-THIRD: struct AAA { };
// CHECK-THIRD: template<typename T>
// CHECK-THIRD: struct AAA<true, T *>
// CHECK-THIRD: { };
// CHECK-THIRD: template< typename>
// CHECK-THIRD: struct BBB {};
// CHECK-THIRD: template<>
// CHECK-THIRD: struct BBB< struct AAA<true, unsigned int> > {};

// CHECK-FOURTH: template<bool  , typename>
// CHECK-FOURTH: struct AAA { };
// CHECK-FOURTH: template<typename T>
// CHECK-FOURTH: struct AAA<true, T *>
// CHECK-FOURTH: { };
// CHECK-FOURTH: template<typename>
// CHECK-FOURTH: struct BBB {};
// CHECK-FOURTH: template<typename T>
// CHECK-FOURTH: struct BBB<T & > {};

