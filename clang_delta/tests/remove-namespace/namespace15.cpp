// RUN: %clang_delta --transformation=remove-namespace --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-FIRST
// RUN: %clang_delta --transformation=remove-namespace --counter=2 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-SECOND
// RUN: %clang_delta --transformation=remove-namespace --counter=3 %s 2>&1 | %remove_lit_checks | FileCheck %s -check-prefix=CHECK-THIRD

namespace NS1 {
template < typename T > struct A {
  typedef const T & ptype;
};
template < typename T > struct B {
  typedef typename NS1::A < T >::ptype ptype;
};
namespace NS2 {
namespace NS3 {
  template < typename > struct C { };
  struct F:C < int > { F () { } };
  F const m2 = F ();
  C < int >m1 = C < int >();
}
namespace NS3 {
class G:public NS1::NS2::NS3::C < int > {
public:
  NS1::NS2::NS3::C < int >base_t;
  typedef NS1::B < NS1::NS2::NS3::F >::ptype MY_T;
  G (MY_T p1):subj (p1) { }
  NS1::NS2::NS3::F subj;
};
template < typename A, typename B >
NS1::NS2::NS3::C < int >operator>> (C < A > const &, C < B >);
template < typename A, typename B >
NS1::NS2::NS3::C < int >operator| (C < A >, C < B >);
template < typename A >
NS1::NS2::NS3::C < int >operator| (C < A >, int);
template < typename A >
NS1::NS2::NS3::C < int >operator| (C < A >, char *);
C < int >eps_p = C < int >();
}
}
}
namespace cl = NS1::NS2::NS3;
struct H:cl::G {
    H (NS1::NS2::NS3::F const &p1) : cl::G (p1) { }
};
struct D {
  H operator[] (NS1::NS2::NS3::F const &p1) {
    return p1;
  }
};
D var = D ();
namespace NS1 {
namespace NS2 {
namespace NS3 {
  template < typename T > C < int > abc (T);
}
}
}
void foo () {
  var[cl::m2] >> cl::abc (0) >> cl::eps_p >> cl::
                          m1;
}

// CHECK-FIRST-NOT: namespace NS1 {
// CHECK-FIRST:     template < typename T > struct A {
// CHECK-FIRST:       typedef const T & ptype;
// CHECK-FIRST      };
// CHECK-FIRST:     template < typename T > struct B {
// CHECK-FIRST:       typedef typename A < T >::ptype ptype;
// CHECK-FIRST:     };
// CHECK-FIRST:     namespace NS2 {
// CHECK-FIRST:     namespace NS3 {
// CHECK-FIRST:       template < typename > struct C { };
// CHECK-FIRST:       struct F:C < int > { F () { } };
// CHECK-FIRST:       F const m2 = F ();
// CHECK-FIRST:       C < int >m1 = C < int >();
// CHECK-FIRST:     }
// CHECK-FIRST:     namespace NS3 {
// CHECK-FIRST:     class G:public NS2::NS3::C < int > {
// CHECK-FIRST:     public:
// CHECK-FIRST:       NS2::NS3::C < int >base_t;
// CHECK-FIRST:       typedef B < NS2::NS3::F >::ptype MY_T;
// CHECK-FIRST:       G (MY_T p1):subj (p1) { }
// CHECK-FIRST:       NS2::NS3::F subj;
// CHECK-FIRST:     };
// CHECK-FIRST:     template < typename A, typename B >
// CHECK-FIRST:     NS2::NS3::C < int >operator>> (C < A > const &, C < B >);
// CHECK-FIRST:     template < typename A, typename B >
// CHECK-FIRST:     NS2::NS3::C < int >operator| (C < A >, C < B >);
// CHECK-FIRST:     template < typename A >
// CHECK-FIRST:     NS2::NS3::C < int >operator| (C < A >, int);
// CHECK-FIRST:     template < typename A >
// CHECK-FIRST:     NS2::NS3::C < int >operator| (C < A >, char *);
// CHECK-FIRST:     C < int >eps_p = C < int >();
// CHECK-FIRST:     }
// CHECK-FIRST:     }
// CHECK-FIRST-NOT: }
// CHECK-FIRST:     namespace cl = NS2::NS3;
// CHECK-FIRST:     struct H:cl::G {
// CHECK-FIRST:         H (NS2::NS3::F const &p1) : cl::G (p1) { }
// CHECK-FIRST:     };
// CHECK-FIRST:     struct D {
// CHECK-FIRST:       H operator[] (NS2::NS3::F const &p1) {
// CHECK-FIRST:         return p1;
// CHECK-FIRST:       }
// CHECK-FIRST:     };
// CHECK-FIRST:     D var = D ();
// CHECK-FIRST-NOT: namespace NS1
// CHECK-FIRST:     namespace NS2 {
// CHECK-FIRST:     namespace NS3 {
// CHECK-FIRST:       template < typename T > C < int > abc (T);
// CHECK-FIRST:     }
// CHECK-FIRST:     }
// CHECK-FIRST-NOT: }
// CHECK-FIRST:     void foo () {
// CHECK-FIRST:       var[cl::m2] >> cl::abc (0) >> cl::eps_p >> cl::
// CHECK-FIRST:                               m1;
// CHECK-FIRST:     }
 
// CHECK-SECOND:     namespace NS1 {
// CHECK-SECOND:     template < typename T > struct A {
// CHECK-SECOND:       typedef const T & ptype;
// CHECK-SECOND:     };
// CHECK-SECOND:     template < typename T > struct B {
// CHECK-SECOND:       typedef typename NS1::A < T >::ptype ptype;
// CHECK-SECOND:     };
// CHECK-SECOND-NOT: namespace NS2
// CHECK-SECOND:     namespace NS3 {
// CHECK-SECOND:       template < typename > struct C { };
// CHECK-SECOND:       struct F:C < int > { F () { } };
// CHECK-SECOND:       F const m2 = F ();
// CHECK-SECOND:       C < int >m1 = C < int >();
// CHECK-SECOND:     }
// CHECK-SECOND:     namespace NS3 {
// CHECK-SECOND:     class G:public NS1::NS3::C < int > {
// CHECK-SECOND:     public:
// CHECK-SECOND:       NS1::NS3::C < int >base_t;
// CHECK-SECOND:       typedef NS1::B < NS1::NS3::F >::ptype MY_T;
// CHECK-SECOND:       G (MY_T p1):subj (p1) { }
// CHECK-SECOND:       NS1::NS3::F subj;
// CHECK-SECOND:     };
// CHECK-SECOND:     template < typename A, typename B >
// CHECK-SECOND:     NS1::NS3::C < int >operator>> (C < A > const &, C < B >);
// CHECK-SECOND:     template < typename A, typename B >
// CHECK-SECOND:     NS1::NS3::C < int >operator| (C < A >, C < B >);
// CHECK-SECOND:     template < typename A >
// CHECK-SECOND:     NS1::NS3::C < int >operator| (C < A >, int);
// CHECK-SECOND:     template < typename A >
// CHECK-SECOND:     NS1::NS3::C < int >operator| (C < A >, char *);
// CHECK-SECOND:     C < int >eps_p = C < int >();
// CHECK-SECOND:     }
// CHECK-SECOND-NOT: }
// CHECK-SECOND:     }
// CHECK-SECOND:     namespace cl = NS1::NS3;
// CHECK-SECOND:     struct H:cl::G {
// CHECK-SECOND:         H (NS1::NS3::F const &p1) : cl::G (p1) { }
// CHECK-SECOND:     };
// CHECK-SECOND:     struct D {
// CHECK-SECOND:       H operator[] (NS1::NS3::F const &p1) {
// CHECK-SECOND:         return p1;
// CHECK-SECOND:       }
// CHECK-SECOND:     };
// CHECK-SECOND:     D var = D ();
// CHECK-SECOND:     namespace NS1 {
// CHECK-SECOND-NOT: namespace NS2
// CHECK-SECOND:     namespace NS3 {
// CHECK-SECOND:       template < typename T > C < int > abc (T);
// CHECK-SECOND:     }
// CHECK-SECOND-NOT: }
// CHECK-SECOND:     }
// CHECK-SECOND:     void foo () {
// CHECK-SECOND:       var[cl::m2] >> cl::abc (0) >> cl::eps_p >> cl::
// CHECK-SECOND:                               m1;
// CHECK-SECOND:     }

// CHECK-THIRD:     namespace NS1 {
// CHECK-THIRD:     template < typename T > struct A {
// CHECK-THIRD:       typedef const T & ptype;
// CHECK-THIRD:     };
// CHECK-THIRD:     template < typename T > struct B {
// CHECK-THIRD:       typedef typename NS1::A < T >::ptype ptype;
// CHECK-THIRD:     };
// CHECK-THIRD:     namespace NS2 {
// CHECK-THIRD-NOT: namespace NS3
// CHECK-THIRD:       template < typename > struct C { };
// CHECK-THIRD:       struct F:C < int > { F () { } };
// CHECK-THIRD:       F const m2 = F ();
// CHECK-THIRD:       C < int >m1 = C < int >();
// CHECK-THRID-NOT: }
// CHECK-THIRD-NOT: namespace NS3
// CHECK-THIRD:     class G:public NS1::NS2::C < int > {
// CHECK-THIRD:     public:
// CHECK-THIRD:       NS1::NS2::C < int >base_t;
// CHECK-THIRD:       typedef NS1::B < NS1::NS2::F >::ptype MY_T;
// CHECK-THIRD:       G (MY_T p1):subj (p1) { }
// CHECK-THIRD:       NS1::NS2::F subj;
// CHECK-THIRD:     };
// CHECK-THIRD:     template < typename A, typename B >
// CHECK-THIRD:     NS1::NS2::C < int >operator>> (C < A > const &, C < B >);
// CHECK-THIRD:     template < typename A, typename B >
// CHECK-THIRD:     NS1::NS2::C < int >operator| (C < A >, C < B >);
// CHECK-THIRD:     template < typename A >
// CHECK-THIRD:     NS1::NS2::C < int >operator| (C < A >, int);
// CHECK-THIRD:     template < typename A >
// CHECK-THIRD:     NS1::NS2::C < int >operator| (C < A >, char *);
// CHECK-THIRD:     C < int >eps_p = C < int >();
// CHECK-THIRD-NOT: }
// CHECK-THIRD:     }
// CHECK-THIRD:     }
// CHECK-THIRD:     namespace cl = NS1::NS2;
// CHECK-THIRD:     struct H:cl::G {
// CHECK-THIRD:         H (NS1::NS2::F const &p1) : cl::G (p1) { }
// CHECK-THIRD:     };
// CHECK-THIRD:     struct D {
// CHECK-THIRD:       H operator[] (NS1::NS2::F const &p1) {
// CHECK-THIRD:         return p1;
// CHECK-THIRD:       }
// CHECK-THIRD:     };
// CHECK-THIRD:     D var = D ();
// CHECK-THIRD:     namespace NS1 {
// CHECK-THIRD:     namespace NS2 {
// CHECK-THIRD-NOT: namespace NS3
// CHECK-THIRD:       template < typename T > C < int > abc (T);
// CHECK-THIRD-NOT: }
// CHECK-THIRD:     }
// CHECK-THIRD:     }
// CHECK-THIRD:     void foo () {
// CHECK-THIRD:       var[cl::m2] >> cl::abc (0) >> cl::eps_p >> cl::
// CHECK-THIRD:                               m1;
// CHECK-THIRD:     }

