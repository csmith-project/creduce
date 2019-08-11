// RUN: %clang_delta --transformation=aggregate-to-scalar --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

extern "C" {
  extern char *gnu_optarg;
// CHECK: char gnu_optarg_0;
}

char foo() {
// CHECK: return gnu_optarg_0;
 return gnu_optarg[0];
}
