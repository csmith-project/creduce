// RUN: %clang_delta --query-instances=param-to-local %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: Available transformation instances: 0

struct S {
  S(int &t) = delete;
};

