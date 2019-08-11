// RUN: %clang_delta --transformation=return-void --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: void strcat (char *dest, const char *src) { }
char *strcat (char *dest, const char *src) { }
