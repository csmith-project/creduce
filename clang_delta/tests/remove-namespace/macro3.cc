// RUN: %clang_delta --transformation=remove-namespace --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: #define START_NS namespace NS1 { namespace NS2 {
#define START_NS namespace NS1 { namespace NS2 {
// CHECK: #define END_NS  } }
#define END_NS  } }
// CHECK-NOT: START_NS
START_NS
// CHECK: void copy1();
  void copy1();
// CHECK-NOT: END_NS
END_NS
// CHECK: #define NS3 NS1::NS2
#define NS3 NS1::NS2
// CHECK: void foo() {
void foo() {
// CHECK: copy1();
  NS3::copy1();
// CHECK: }
}
