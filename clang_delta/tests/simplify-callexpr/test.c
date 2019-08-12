// RUN: %clang_delta --transformation=simplify-callexpr --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

float a;
// CHECK: int b[(0,0) == 1];
int b[__builtin_classify_type(a + 1i) == 1];
