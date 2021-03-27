// RUN: %clang_delta --transformation=simple-inliner --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: struct S {
struct S {
// CHECK-NOT: template
  template <typename T>
// CHECK-NOT: T bar 
  T bar(T a) {
    return a;
  }
// CHECK: };
};

void foo() {
  S s;
  int x = s.bar<int>(1);
}

// CHECK: void foo() {
// CHECK: int [[VAR:.*]];
// CHECK: S s;
// CHECK: int a = 1;
// CHECK: [[VAR]] =  a;
// CHECK: int x = [[VAR]];
// CHECK: }
