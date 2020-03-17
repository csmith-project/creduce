// RUN: %clang_delta --transformation=remove-unused-function --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s
// RUN: %clang_delta --transformation=remove-unused-function --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

// CHECK: {{^;}}
// CHECK-SECOND: template<typename T> void foo(T *);
template<typename T> void foo(T *);
// CHECK: template <typename T> struct S {
// CHECK-SECOND: template <typename T> struct S {
template <typename T> struct S {
// CHECK: template<typename T1> friend void foo(T1 *);
// CHECK-SECOND: {{ *}};
  template<typename T1> friend void foo(T1 *);
// CHECK: };
// CHECK-SECOND: };
};
