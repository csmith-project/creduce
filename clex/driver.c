#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "stuff.h"

struct tok_t {
  char *str;
  enum tok_kind kind;
  int id;
};

int next_id;
struct tok_t *tok_list;
int toks;
int max_toks;
const int initial_length = 1;

int add_tok (char *str, enum tok_kind kind)
{
  assert (str);
  if (toks >= max_toks) {
    max_toks *= 2;
    tok_list = (struct tok_t *) realloc (tok_list, max_toks * sizeof (struct tok_t));
    assert (tok_list);
  }
  tok_list[toks].str = strdup (str);
  assert (tok_list[toks].str);
  tok_list[toks].kind = kind;
  tok_list[toks].id = -1;
  toks++;
  return toks-1;
}

void classify_tok (int tok)
{
  if (tok_list[tok].kind != TOK_IDENT) return;
  int i;
  for (i=0; i<toks; i++) {
    if (tok_list[i].kind != TOK_IDENT) continue;
    if (i==tok) continue;
    assert (tok_list[i].id != -1);
    if (strcmp (tok_list[i].str, tok_list[tok].str) == 0) {
      tok_list[tok].id = tok_list[i].id;
      return;
    }
  }
  tok_list[tok].id = next_id;
  next_id++;
}

void doit (enum tok_kind kind)
{
  int tok = add_tok (yytext, kind);
  classify_tok (tok);
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
    printf ("%s", tok_list[i].str);
    if (tok_list[i].id != -1) {
      printf (" -=- %d -=-", tok_list[i].id);
    }
    printf ("\n");
  }
  printf ("// %d tokens\n", count);
  return 0;
}
