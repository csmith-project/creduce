int printf (const char *, ...);
#pragma pack(1)
struct {
  int f0;
  char f2;
  int:16;
  char f4;
  int f5;
  int f6;
  int f7:20;
}
a = {
  1
};
struct {
  int f0;
}
c;
static char b = 1;
int d, e;
int
main () {
  for (; e <= 0; e += 1) {
    a.f7 = b = 0;
    c = c;
  }
  d = 0;
  printf ("%d\n", b);
  return 0;
}
