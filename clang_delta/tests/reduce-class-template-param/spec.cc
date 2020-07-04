// RUN: %clang_delta --transformation=reduce-class-template-param --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=reduce-class-template-param --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

template<typename T1, typename T2>
struct S{};
typedef struct S<int, int> new_type;
struct S<int, int> s;

// CHECK-FIRST: template< typename T2>
// CHECK-FIRST: struct S{};
// CHECK-FIRST: typedef struct S< int> new_type;
// CHECK-FIRST: struct S< int> s;

// CHECK-SECOND: template<typename T1>
// CHECK-SECOND: struct S{};
// CHECK-SECOND: typedef struct S<int> new_type;
// CHECK-SECOND: struct S<int> s;
