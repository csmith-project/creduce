// RUN: %clang_delta --query-instances=copy-propagation %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: Available transformation instances: 0

void foo() {
  int b =  ((unsigned char *)foo)[0];
}
