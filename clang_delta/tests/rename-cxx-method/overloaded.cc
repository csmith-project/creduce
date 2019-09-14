// RUN: %clang_delta --transformation=rename-cxx-method --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

template < class> struct k  {
// CHECK: template < class l > void operator=(int);
  template < class l > void operator=(int);
};
// CHECK: struct G { void m_fn1() { } };
struct G { void p() { } };
template < class i>
template < class l>
// CHECK: k<i>::k() { operator=(1); }
k<i>::k() { operator=(1); }
