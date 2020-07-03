// RUN: %clang_delta --query-instances=template-arg-to-int %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: Available transformation instances: 0

struct v { int ak; };
template <typename> class aa;
template <typename> struct j;
template <typename b> struct j<aa<b>> {
  using k = b;
};
struct ag {
  typename j<aa<v>>::k
  foo() { return s; }
  v s;
};
void as() {
  ag aw;
  aw.foo().ak;
}
