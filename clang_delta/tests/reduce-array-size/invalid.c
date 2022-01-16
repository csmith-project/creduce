// RUN: %clang_delta --transformation=reduce-array-size --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: int a[1] = {[72057594037927936] 0};
int a[] = {[72057594037927936] 0};
void main() {}
