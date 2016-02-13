//RUN: %clang_delta --transformation=template-non-type-arg-to-int --counter=1 %s | FileCheck %s

enum _Lock_policy { _S_atomic } const __default_lock_policy = _S_atomic;
template <_Lock_policy _Lp> class __shared_count {
public:
  template <typename _Ptr> __shared_count(_Ptr __p) { }
};

//CHECK: template <int _Lp> class __shared_count {
//CHECK-NEXT: public:
//CHECK-NEXT:   template <typename _Ptr> __shared_count(_Ptr __p) { }
//CHECK-NEXT: };
