// RUN: %clang_delta --transformation=empty-struct-to-int --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

struct S{};
struct S1 : S {};
// CHECK-NOT: struct S3
struct S3 {};

