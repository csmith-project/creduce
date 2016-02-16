//RUN: %clang_delta --transformation=remove-trivial-base-template --counter=1 %s | FileCheck %s
namespace std {
  template <typename T, typename U> class iterator{};
}
template<typename T, typename U>
struct TriaRawIterator : public std::iterator<T, U> {
  int pinHere;
};

//CHECK: namespace std {
//CHECK-NEXT:  template <typename T, typename U> class iterator{};
//CHECK-NEXT: }
//CHECK-NEXT: template<typename T, typename U>
//CHECK-NEXT: struct TriaRawIterator  {
//CHECK-NEXT:   int pinHere;
//CHECK-NEXT: };
