#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

void doit (void);

int main(int argc, char *argv[]) {
  assert (argc == 2);
  printf ("file = '%s'\n", argv[1]);
  FILE *in = fopen (argv[1], "r");
  assert (in);
  yyin = in;   
  yylex();
  printf ("%d\n", count);
  return 0;
}
