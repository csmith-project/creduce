// RUN: %clang_delta --transformation=simple-inliner --counter=1 %s 2>&1 | %remove_lit_checks | FileCheck %s

struct S2 {
  int g_313;
  char g_350;
};
// CHECK: struct S2 {
// CHECK:   int g_313;
// CHECK:   char g_350;
// CHECK: };

// CHECK-NOT: func_1
void func_1(struct S2 *p_886) {
  char *l_349 = &p_886->g_350;
  *l_349 = ((short)p_886->g_313 >= 6UL);
}
void entry(long *result, int sequence_input) {
  struct S2 c_887;
  struct S2 *p_886 = &c_887;
  struct S2 c_888 = {-8L};
  c_887 = c_888;
  func_1(p_886);
  result[0] = p_886->g_350;
}

// CHECK: void entry(long *result, int sequence_input) {
// CHECK:   struct S2 c_887;
// CHECK:   struct S2 *p_886 = &c_887;
// CHECK:   struct S2 c_888 = {-8L};
// CHECK:   c_887 = c_888;
// CHECK:   {
// CHECK:   struct S2 *[[VAR:.*]] = p_886;
// CHECK:   {struct S2 *p_886 = [[VAR]];
// CHECK:     char *l_349 = &p_886->g_350;
// CHECK:     *l_349 = ((short)p_886->g_313 >= 6UL);
// CHECK:   }}
// CHECK:   result[0] = p_886->g_350;
// CHECK: }
