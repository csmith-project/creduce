// RUN: %clang_delta --query-instances=simple-inliner %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: Available transformation instances: 0

struct a;
template <typename> using b = a;
struct a {
  template <typename c> auto d(c) {
    using e = b<c>;
    return e{};
  }
} f;
auto g() -> a {
  a h;
  f.d([] {}).d(g());
  return h;
}
