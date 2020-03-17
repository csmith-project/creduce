// RUN: %clang_delta --transformation=remove-unused-function --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: #define __LEAF, __leaf__
#define __LEAF, __leaf__
// CHECK: #define __THROW __attribute__((__nothrow__ __LEAF))
#define __THROW __attribute__((__nothrow__ __LEAF))
// CHECK: # define __nonnull(params) __attribute__ ((__nonnull__ params))
# define __nonnull(params) __attribute__ ((__nonnull__ params))

// CHECK-NOT: extern void *foo(void* __restrict p1
extern void *foo(void* __restrict p1, const void* __restrict p2,
// CHECK-NOT: int p3, unsigned p4)
                 int p3, unsigned p4)
// CHECK-NOT: __THROW __nonnull ((1, 2));
     __THROW __nonnull ((1, 2));
