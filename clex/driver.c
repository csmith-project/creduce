#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "stuff.h"

struct tok_t {
  char *str;
};

struct tok_t *tok_list;
int toks;
int max_toks;
const int initial_length = 1;

void add_tok (char *str)
{
  if (toks >= max_toks) {
    max_toks *= 2;
    tok_list = (struct tok_t *) realloc (tok_list, max_toks * sizeof (struct tok_t));
    assert (tok_list);
  }
  tok_list[toks].str = strdup (str);
  assert (tok_list[toks].str);
  toks++;
}

void doit (enum tok_kind kind)
{
  add_tok (yytext);
  count++;
}

int main(int argc, char *argv[]) {
  assert (argc == 2);
  printf ("file = '%s'\n", argv[1]);
  FILE *in = fopen (argv[1], "r");
  assert (in);
  yyin = in;   

  max_toks = initial_length;
  tok_list = (struct tok_t *) malloc (max_toks * sizeof (struct tok_t));
  assert (tok_list);

  yylex();
  int i;
  for (i=0; i<toks; i++) {
    printf ("%s ", tok_list[i].str);
  }
  printf ("// %d tokens\n", count);
  return 0;
}
