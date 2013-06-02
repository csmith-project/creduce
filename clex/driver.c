#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

/*
 * Stuff defined for us by the lex/flex-generated code.
 */
extern FILE *yyin;
extern int yylex(void);
extern char *yytext;

/*
 * Stuff that we define within the lex/flex-generated code.
 */
extern int count;
extern void doit(void);

char **tok_list;
int toks;
int max_toks;
const int initial_length = 1;

void add_tok (char *tp)
{
  if (toks >= max_toks) {
    max_toks *= 2;
    tok_list = realloc (tok_list, max_toks * sizeof (char *));
    assert (tok_list);
  }
  tok_list[toks] = strdup (yytext);
  assert (tok_list[toks]);
  toks++;
}

void doit (void)
{
  add_tok (yytext);
  // printf ("%s\n", yytext);
  count++;
}

int main(int argc, char *argv[]) {
  assert (argc == 2);
  printf ("file = '%s'\n", argv[1]);
  FILE *in = fopen (argv[1], "r");
  assert (in);
  yyin = in;   

  max_toks = initial_length;
  tok_list = (char **) malloc (max_toks * sizeof (char *));
  assert (tok_list);

  yylex();
  int i;
  for (i=0; i<toks; i++) {
    printf ("%s ", tok_list[i]);
  }
  printf ("// %d tokens\n", count);
  return 0;
}
