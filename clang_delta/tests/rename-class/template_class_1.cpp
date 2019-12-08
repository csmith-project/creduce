// RUN: %clang_delta --transformation=rename-class --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: template class A<Traits<X>>;
template class Base<Traits<X>>;
