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
int max_seen = -1;

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
  int id;
  // FIXME do not hardcode this...
  int res = sscanf (tok_list[tok].str, "_x_%d", &id);
  if (res==1) {
    if (id > max_seen) {
      max_seen = id;
    }
  }

  // FIXME: have a C++ mode that avoids trying to rename C++ keywords

  // FIXME-- this keeps us out of transformation loops until I
  // implement something smarter
  if (strncmp (tok_list[tok].str, "_x_", 3) == 0 ||
      strlen (tok_list[tok].str) <= 3) return;

  // FIXME-- this loop makes overall perforamnce quadratic, better
  // not run this program on big inputs

  int i;
  for (i=0; i<toks; i++) {
    if (tok_list[i].kind != TOK_IDENT) continue;
    if (i==tok) continue;
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

enum mode_t {
  MODE_RENAME = 1111,
  MODE_RENAME_DEBUG,
  MODE_NONE,
};

void dump_renamed_file (int tok_index)
{
  int i;
  int matched = 0;
  for (i=0; i<toks; i++) {
    int printed = 0;
    if (tok_list[i].id != -1) {
      if (tok_list[i].id == tok_index) {
	printf ("_x_%d", max_seen+1);
	printed = 1;
	matched = 1;
      }
      // printf (" -=- %d -=-", tok_list[i].id);
    }
    if (!printed) printf ("%s", tok_list[i].str);
  }
  if (matched) {
    exit (0);
  } else {
    exit (-1);
  }
}

int main(int argc, char *argv[]) {
  assert (argc == 4);

  char *cmd = argv[1];
  enum mode_t mode = MODE_NONE;
  if (strcmp (cmd, "rename-toks") == 0) {
    mode = MODE_RENAME;
  } else if (strcmp (cmd, "rename-toks-debug") == 0) {
    mode = MODE_RENAME_DEBUG;
  } else {
    printf ("error: unknown mode '%s'\n", cmd);
    exit (-50);
  }

  int tok_index;
  int ret = sscanf (argv[2], "%d", &tok_index);
  assert (ret==1);
  // printf ("file = '%s'\n", argv[3]);
  FILE *in = fopen (argv[3], "r");
  assert (in);
  yyin = in;

  max_toks = initial_length;
  tok_list = (struct tok_t *) malloc (max_toks * sizeof (struct tok_t));
  assert (tok_list);

  yylex();

  // these calls all exit() at the end
  switch (mode) {
  case MODE_RENAME:
    dump_renamed_file (tok_index);
  }
  assert (0);
}
