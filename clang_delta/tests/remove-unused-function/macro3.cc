// RUN: %clang_delta --transformation=remove-unused-function --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: # define BEGIN_NAMESPACE_STD namespace std {
# define BEGIN_NAMESPACE_STD namespace std {
// CHECK: # define END_NAMESPACE_STD }
# define END_NAMESPACE_STD }
// CHECK: # define __THROW throw ()
# define __THROW throw ()
// CHECK: # define __nonnull(params)
# define __nonnull(params)

// CHECK: BEGIN_NAMESPACE_STD
BEGIN_NAMESPACE_STD
// CHECK-NOT: extern void *foo(void *__restrict p1,
extern void *foo(void *__restrict p1,
// CHECK-NOT: unsigned p2) __THROW __nonnull
                 unsigned p2) __THROW __nonnull ((1, 2));
// CHECK: END_NAMESPACE_STD
END_NAMESPACE_STD
