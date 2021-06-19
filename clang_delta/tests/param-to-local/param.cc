// RUN: %clang_delta --transformation=param-to-local --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-1
// RUN: %clang_delta --transformation=param-to-local --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-2
// RUN: %clang_delta --transformation=param-to-local --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-3
// RUN: %clang_delta --transformation=param-to-local --counter=4 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-4
// RUN: %clang_delta --transformation=param-to-local --counter=5 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-5
// RUN: %clang_delta --transformation=param-to-local --counter=6 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-6
// RUN: %clang_delta --transformation=param-to-local --counter=7 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-7
// RUN: %clang_delta --transformation=param-to-local --counter=8 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-8
// RUN: %clang_delta --transformation=param-to-local --counter=9 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-9
// RUN: %clang_delta --transformation=param-to-local --counter=10 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-10
// RUN: %clang_delta --transformation=param-to-local --counter=11 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-11
// RUN: %clang_delta --transformation=param-to-local --counter=12 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-12
// RUN: %clang_delta --transformation=param-to-local --counter=13 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-13
// RUN: %clang_delta --transformation=param-to-local --counter=14 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-14
// RUN: %clang_delta --transformation=param-to-local --counter=15 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-15
// RUN: %clang_delta --transformation=param-to-local --counter=16 %s 2>&1 | %remove_lit_checks | FileCheck %s --check-prefix=CHECK-16

void foo1(void *) {}
void foo2( int * ) {}
void foo3( int* ) {}
void foo4(int*) {}
void foo5(int, int) {}
void foo6(int x, int) {}
void foo7(int x, int y = 111) {}
void foo8(int x, int y = 123, int z = 244) {}
void foo9(int x, int, int z = 111) {}

// CHECK-1: void foo1(void) {}
// CHECK-1: void foo2( int * ) {}

// CHECK-2: void foo1(void *) {}
// CHECK-2: void foo2( void ) {}
// CHECK-2: void foo3( int* ) {}

// CHECK-3: void foo2( int * ) {}
// CHECK-3: void foo3( void ) {}
// CHECK-3: void foo4(int*) {}

// CHECK-4: void foo3( int* ) {}
// CHECK-4: void foo4(void) {}
// CHECK-4: void foo5(int, int) {}

// CHECK-5: void foo4(int*) {}
// CHECK-5: void foo5( int) {}
// CHECK-5: void foo6(int x, int) {}

// CHECK-6: void foo4(int*) {}
// CHECK-6: void foo5(int) {}
// CHECK-6: void foo6(int x, int) {}

// CHECK-7: void foo5(int, int) {}
// CHECK-7: void foo6( int) {
// CHECK-7:     int x = 0;}
// CHECK-7: void foo7(int x, int y = 111) {}

// CHECK-8: void foo5(int, int) {}
// CHECK-8: void foo6(int x) {}
// CHECK-8: void foo7(int x, int y = 111) {}

// CHECK-9: void foo6(int x, int) {}
// CHECK-9: void foo7( int y = 111) {
// CHECK-9:     int x = 0;}
// CHECK-9: void foo8(int x, int y = 123, int z = 244) {}

// CHECK-10: void foo6(int x, int) {}
// CHECK-10: void foo7(int x) {
// CHECK-10:     int y = 111;}
// CHECK-10: void foo8(int x, int y = 123, int z = 244) {}

// CHECK-11: void foo7(int x, int y = 111) {}
// CHECK-11: void foo8( int y = 123, int z = 244) {
// CHECK-11:     int x = 0;}
// CHECK-11: void foo9(int x, int, int z = 111) {}

// CHECK-12: void foo7(int x, int y = 111) {}
// CHECK-12: void foo8(int x,  int z = 244) {
// CHECK-12:     int y = 123;}
// CHECK-12: void foo9(int x, int, int z = 111) {}

// CHECK-13: void foo7(int x, int y = 111) {}
// CHECK-13: void foo8(int x, int y = 123) {
// CHECK-13:     int z = 244;}
// CHECK-13: void foo9(int x, int, int z = 111) {}

// CHECK-14: void foo8(int x, int y = 123, int z = 244) {}
// CHECK-14: void foo9( int, int z = 111) {
// CHECK-14:     int x = 0;}

// CHECK-15: void foo8(int x, int y = 123, int z = 244) {}
// CHECK-15: void foo9(int x,  int z = 111) {}

// CHECK-16: void foo8(int x, int y = 123, int z = 244) {}
// CHECK-16: void foo9(int x, int) {
// CHECK-16:     int z = 111;}

