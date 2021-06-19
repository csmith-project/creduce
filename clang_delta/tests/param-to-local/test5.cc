// RUN: %clang_delta --query-instances=param-to-local %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: Available transformation instances: 0

class S1 {
  static void foo();
  static void bar(void*);
};
class S2 {
 public:
  void baz();
};
void S1::foo() {
  auto t = new S2;
  &S2::baz, [t] { bar(t); };
}
