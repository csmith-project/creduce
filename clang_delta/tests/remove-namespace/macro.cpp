// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-namespace --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=remove-namespace --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD
// RUN: %clang_delta --transformation=remove-namespace --counter=4 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FOURTH
// RUN: %clang_delta --transformation=remove-namespace --counter=5 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIFTH

#define BEGIN_NAMESPACE namespace ns {
#define END_NAMESPACE }
BEGIN_NAMESPACE
int foo();
END_NAMESPACE
#define BEGIN_NAMESPACE2 namespace ns2 {
BEGIN_NAMESPACE2
int foo2();
}
#define END_NAMESPACE3 }
namespace ns3 {
int foo3();
END_NAMESPACE3
#define BEGIN_NAMESPACE4 namespace ns4 { namespace ns5 {
#define END_NAMESPACE4  } }
BEGIN_NAMESPACE4
void foo4();
END_NAMESPACE4

// CHECK-FIRST:      #define BEGIN_NAMESPACE namespace ns {
// CHECK-FIRST:      #define END_NAMESPACE }
// CHECK-FIRST-NOT:  BEGIN_NAMESPACE
// CHECK-FIRST:      int foo();
// CHECK-FIRST-NOT:  END_NAMESPACE
// CHECK-FIRST:      #define BEGIN_NAMESPACE2 namespace ns2 {
// CHECK-FIRST:      BEGIN_NAMESPACE2
// CHECK-FIRST:      int foo2();
// CHECK-FIRST:      }
// CHECK-FIRST:      #define END_NAMESPACE3 }
// CHECK-FIRST:      namespace ns3 {
// CHECK-FIRST:      int foo3();
// CHECK-FIRST:      END_NAMESPACE3
// CHECK-FIRST:      #define BEGIN_NAMESPACE4 namespace ns4 { namespace ns5 {
// CHECK-FIRST:      #define END_NAMESPACE4  } }
// CHECK-FIRST:      BEGIN_NAMESPACE4
// CHECK-FIRST:      void foo4();
// CHECK-FIRST:      END_NAMESPACE4

// CHECK-SECOND:     #define BEGIN_NAMESPACE namespace ns {
// CHECK-SECOND:     #define END_NAMESPACE }
// CHECK-SECOND:     BEGIN_NAMESPACE
// CHECK-SECOND:     int foo();
// CHECK-SECOND:     END_NAMESPACE
// CHECK-SECOND:     #define BEGIN_NAMESPACE2 namespace ns2 {
// CHECK-SECOND-NOT: BEGIN_NAMESPACE2
// CHECK-SECOND:     int foo2();
// CHECK-SECOND-NOT: }
// CHECK-SECOND:     #define END_NAMESPACE3 }
// CHECK-SECOND:     namespace ns3 {
// CHECK-SECOND:     int foo3();
// CHECK-SECOND:     END_NAMESPACE3
// CHECK-SECOND:     #define BEGIN_NAMESPACE4 namespace ns4 { namespace ns5 {
// CHECK-SECOND:     #define END_NAMESPACE4  } }
// CHECK-SECOND:     BEGIN_NAMESPACE4
// CHECK-SECOND:     void foo4();
// CHECK-SECOND:     END_NAMESPACE4

// CHECK-THIRD:      #define BEGIN_NAMESPACE namespace ns {
// CHECK-THIRD:      #define END_NAMESPACE }
// CHECK-THIRD:      BEGIN_NAMESPACE
// CHECK-THIRD:      int foo();
// CHECK-THIRD:      END_NAMESPACE
// CHECK-THIRD:      #define BEGIN_NAMESPACE2 namespace ns2 {
// CHECK-THIRD:      BEGIN_NAMESPACE2
// CHECK-THIRD:      int foo2();
// CHECK-THIRD:      }
// CHECK-THIRD:      #define END_NAMESPACE3 }
// CHECK-THIRD-NOT:  namespace ns3 {
// CHECK-THIRD:      int foo3();
// CHECK-THIRD-NOT:  END_NAMESPACE3
// CHECK-THIRD:      #define BEGIN_NAMESPACE4 namespace ns4 { namespace ns5 {
// CHECK-THIRD:      #define END_NAMESPACE4  } }
// CHECK-THIRD:      BEGIN_NAMESPACE4
// CHECK-THIRD:      void foo4();
// CHECK-THIRD:      END_NAMESPACE4

// CHECK-FOURTH:     #define BEGIN_NAMESPACE namespace ns {
// CHECK-FOURTH:     #define END_NAMESPACE }
// CHECK-FOURTH:     BEGIN_NAMESPACE
// CHECK-FOURTH:     int foo();
// CHECK-FOURTH:     END_NAMESPACE
// CHECK-FOURTH:     #define BEGIN_NAMESPACE2 namespace ns2 {
// CHECK-FOURTH:     BEGIN_NAMESPACE2
// CHECK-FOURTH:     int foo2();
// CHECK-FOURTH:     }
// CHECK-FOURTH:     #define END_NAMESPACE3 }
// CHECK-FOURTH:     namespace ns3 {
// CHECK-FOURTH:     int foo3();
// CHECK-FOURTH:     END_NAMESPACE3
// CHECK-FOURTH:     #define BEGIN_NAMESPACE4 namespace ns4 { namespace ns5 {
// CHECK-FOURTH:     #define END_NAMESPACE4  } }
// CHECK-FOURTH-NOT: BEGIN_NAMESPACE4
// CHECK-FOURTH:     void foo4();
// CHECK-FOURTH-NOT: END_NAMESPACE4

// CHECK-FIFTH:     #define BEGIN_NAMESPACE namespace ns {
// CHECK-FIFTH:     #define END_NAMESPACE }
// CHECK-FIFTH:     BEGIN_NAMESPACE
// CHECK-FIFTH:     int foo();
// CHECK-FIFTH:     END_NAMESPACE
// CHECK-FIFTH:     #define BEGIN_NAMESPACE2 namespace ns2 {
// CHECK-FIFTH:     BEGIN_NAMESPACE2
// CHECK-FIFTH:     int foo2();
// CHECK-FIFTH:     }
// CHECK-FIFTH:     #define END_NAMESPACE3 }
// CHECK-FIFTH:     namespace ns3 {
// CHECK-FIFTH:     int foo3();
// CHECK-FIFTH:     END_NAMESPACE3
// CHECK-FIFTH:     #define BEGIN_NAMESPACE4 namespace ns4 { namespace ns5 {
// CHECK-FIFTH:     #define END_NAMESPACE4  } }
// CHECK-FIFTH-NOT: BEGIN_NAMESPACE4
// CHECK-FIFTH:     void foo4();
// CHECK-FIFTH-NOT: END_NAMESPACE4

