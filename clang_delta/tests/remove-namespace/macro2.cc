// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

#define BEGIN_NAMESPACE namespace ns1 {
#define END_NAMESPACE }
BEGIN_NAMESPACE
END_NAMESPACE
// CHECK: #define BEGIN_NAMESPACE namespace ns1 {
// CHECK: #define END_NAMESPACE }
// CHECK-NOT: BEGIN_NAMESPACE
// CHECK-NOT: END_NAMESPACE
