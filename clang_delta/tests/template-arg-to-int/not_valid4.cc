// RUN: %clang_delta --query-instances=template-arg-to-int %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: Available transformation instances: 0

template <typename T1> struct pair
{
  typedef T1 first;
};

struct S1 {
  static void foo();
};

struct S2 {
  typedef pair<S1>::first type;
  struct S3 : type {
    void bar() {
      type::foo();
    }
  };
};

