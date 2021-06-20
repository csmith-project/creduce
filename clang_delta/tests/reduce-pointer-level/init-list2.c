// RUN: %clang_delta --transformation=reduce-pointer-level --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

int g1;
int *g2[][1] = {{}, &g1};
int **g3 = &g2;

// CHECK: int g1;
// CHECK: int *g2[][1] = {{[{][{]}}}, &g1};
// CHECK: int *g3 = &g1;

