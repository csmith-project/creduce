// RUN: %clang_delta --transformation=remove-enum-member-value --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: typedef enum E { 
typedef enum E {
// CHECK: a ,
  a = __ATOMIC_RELAXED,
// CHECK: };
};
