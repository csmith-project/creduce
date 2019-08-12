// RUN: %clang_delta --transformation=callexpr-to-value --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

float a;
// CHECK: int b[0 == 9 ? 1 : 2];
int b[__builtin_classify_type(a) == 9 ? 1 : 2];
