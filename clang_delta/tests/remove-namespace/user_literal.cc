// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK-NOT: namespace
inline namespace a {
// CHECK: void operator"" [[NAME:.*]](unsigned long long);
void operator""   _b(unsigned long long);
// CHECK: void {{.*}}foo() {
void foo() {
// CHECK: [[NAME]];
  1_b;
// CHECK: }
}
// CHECK-NOT: }
}

