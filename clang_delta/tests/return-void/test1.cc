// RUN: %clang_delta --transformation=return-void --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

extern "C" extern "C"

// CHECK: void foo ();
int foo ();

int main_res;
main () {
  foo ();
}
