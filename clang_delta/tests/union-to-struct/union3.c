// RUN: %clang_delta --transformation=union-to-struct --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

void f() {
// CHECK: struct {} l;
  union {} l;
}