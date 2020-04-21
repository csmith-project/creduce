// RUN: %clang_delta --query-instances=remove-unused-function %s 2>&1 | %remove_lit_checks | FileCheck %s

// CHECK: Available transformation instances: 0
namespace a {
  inline namespace b {
    using namespace a;
  }
  struct c;
  namespace {
    using a::c;
  }
}
