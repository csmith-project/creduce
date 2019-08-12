// RUN: %clang_delta --transformation=callexpr-to-value --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

#define a(b) (b)
// CHECK: c() { 0; }
c() { d a(0); }
