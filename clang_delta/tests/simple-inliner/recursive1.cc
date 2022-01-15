// RUN: %clang_delta --query-instances=simple-inliner %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: Available transformation instances: 0

int foo(int p) {
  int i = 0;
  return foo(i);
}
