struct S1 {
  int f0
}
a[];
struct S1 fn1 ();
fn2 (void) {
  for (;;)
    break;
  a[0] = fn1 ();
}
struct S1
fn1 (int p1) {
}
main () {
  fn2 ();
  printf ("%d\n", a[0].f0);
  return 0;
}
