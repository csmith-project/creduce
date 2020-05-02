// RUN: %clang_delta --transformation=remove-unused-function --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-unused-function --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND

struct S {
  template <typename T>
  void foo() const;
  template <typename T>
  int operator+(int a) const;
};

// CHECK-FIRST:      struct S {
// CHECK-FIRST-NEXT:   {{^ *; *}}
// CHECK-FIRST-NEXT:   template <typename T>
// CHECK-FIRST-NEXT:   int operator+(int a) const;
// CHECK-FIRST-NEXT: };

// CHECK-SECOND:      struct S {
// CHECK-SECOND-NEXT:   template <typename T>
// CHECK-SECOND-NEXT:   void foo() const;
// CHECK-SECOND-NEXT:   {{^ *; *}}
// CHECK-SECOND-NEXT: };
