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

void doit (enum tok_kind kind)
{
  int tok = add_tok (yytext, kind);
  count++;
}

enum mode_t {
  MODE_RENAME = 1111,
  MODE_PRINT,
  MODE_DELETE_STRING,
  MODE_RM_TOKS,
  MODE_RM_TOK_PATTERN,
  MODE_NONE,
};

void print_toks (void)
{
  int i;
  for (i=0; i<toks; i++) {
    // if (tok_list[i].kind != TOK_WS) printf ("%s\n", tok_list[i].str);
    if (tok_list[i].kind == TOK_IDENT) printf ("%s\n", tok_list[i].str);
  }
  exit (0);
}

// FIXME: have a C++ mode that avoids trying to rename C++ keywords?

void rename_toks (int tok_index)
{
  // find the highest number used in a rename, and also assign a number to each 
  // distinct identifier that doesn't start wtih our prefix
  int max_seen = -1;
  int next_id = 0;
  int i;
  for (i=0; i<toks; i++) {
    if (tok_list[i].kind != TOK_IDENT) continue;
    // FIXME find first unused instead of max+1?
    int id;
    int res = sscanf (tok_list[i].str, "_x_%d", &id);
    if (res==1) {
      // this one was already renamed
      if (id > max_seen) {
	max_seen = id;
      }
    } else {
      // it's a candidate for renaming so give it a number
      // FIXME: can bail out of here if the id exceeds our target
      // tok_index
      int j;
      int matched = 0;
      for (j=0; j<i; j++) {
	if (tok_list[j].kind != TOK_IDENT) continue;
	if (strcmp (tok_list[j].str, tok_list[i].str) == 0) {
	  matched = 1;
	  tok_list[i].id = tok_list[j].id;
	}
      }
      if (!matched) {
	tok_list[i].id = next_id;
	next_id++;
      }
    }
  }

  // now dump the renamed token stream
  int matched = 0;
  for (i=0; i<toks; i++) {
    int printed = 0;
    if (tok_list[i].id != -1 &&
	tok_list[i].id == tok_index) {
      printf ("_x_%d", max_seen+1);
      printed = 1;
      matched = 1;
    }
    if (!printed) printf ("%s", tok_list[i].str);
  }
  if (matched) {
    exit (0);
  } else {
    exit (-1);
  }
}

void delete_string (int idx)
{
  int i;
  int matched = 0;
  int which = 0;
  for (i=0; i<toks; i++) {
    int printed = 0;
    if (tok_list[i].kind == TOK_STRING &&
	strcmp (tok_list[i].str, "\"\"") != 0) {
      if (which == idx) {
	printf ("\"\"");
	printed = 1;
	matched = 1;
      } 
      which++;
    }
    if (!printed) printf ("%s", tok_list[i].str);
  }
  if (matched) {
    exit (0);
  } else {
    exit (-1);
  }
}

int rm_toks_n;

void rm_toks (int idx)
{
  int i;
  int matched = 0;
  int which = 0;
  int started = 0;
  for (i=0; i<toks; i++) {
    if (tok_list[i].kind != TOK_WS) {
      if (which == idx) {
	started = 1;
	matched = 1;
      }
      which++;
    }
    if (!started ||
	(started && which > (idx + rm_toks_n))) printf ("%s", tok_list[i].str);
  }
  if (matched) {
    exit (0);
  } else {
    exit (-1);
  }
}

void print_pattern (unsigned char c)
{
  int z;
  for (z=0; z<8; z++) {
    printf ("%d", (c&128)?1:0);
    c <<= 1;
  }
  printf ("\n");
}

void rm_tok_pattern (int idx)
{
  int i;
  int n_patterns = 1<<(rm_toks_n-1);
  unsigned char patterns[n_patterns];
  for (i=0; i<n_patterns; i++) {
    patterns[i] = 1 | ((unsigned)i << 1);
  }

  int n_pattern = idx & (n_patterns-1);
  unsigned char pat = patterns[n_pattern];	

  if (0) {
    printf ("pattern = ");
    print_pattern (pat);
  }

  idx >>= (rm_toks_n-1);
  
  int which = 0;
  int started = 0;
  int matched = 0;
  int deleted = 0;
  for (i=0; i<toks; i++) {
    if (tok_list[i].kind != TOK_WS) {
      if (which == idx) {
	matched = 1;
	started = 1;
      }
      if (which == (idx + rm_toks_n)) started = 0;
      which++;
    }
    int print = 0;
    int pattern_idx = which - idx;
    if (tok_list[i].kind == TOK_WS) {
      print = 1;
    } else {
      if (!started) {
	print = 1;
      } else {
	if (pat & 1) {
	  deleted = 1;
	  // printf ("[%s]", tok_list[i].str);
	} else {
	  print = 1;
	}
	pat >>= 1;
      }
    }
    if (print) printf ("%s", tok_list[i].str);
  }
  if (matched && deleted) {
    exit (0);
  } else {
    exit (-1);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf ("USAGE: %s command index file\n", argv[0]);
  }

  char *cmd = argv[1];
  enum mode_t mode = MODE_NONE;
  if (strcmp (cmd, "rename-toks") == 0) {
    mode = MODE_RENAME;
  } else if (strcmp (cmd, "print") == 0) {
    mode = MODE_PRINT;
  } else if (strcmp (cmd, "delete-string") == 0) {
    mode = MODE_DELETE_STRING;
  } else if (strncmp (cmd, "rm-toks-", 8) == 0) {
    mode = MODE_RM_TOKS;
    int res = sscanf (&cmd[8], "%d", &rm_toks_n);
    assert (res==1);
    assert (rm_toks_n > 0 && rm_toks_n <= 1000);
  } else if (strncmp (cmd, "rm-tok-pattern-", 15) == 0) {
    mode = MODE_RM_TOK_PATTERN;
    int res = sscanf (&cmd[15], "%d", &rm_toks_n);
    assert (res==1);
    assert (rm_toks_n > 1 && rm_toks_n <= 8);
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
  case MODE_PRINT:
    print_toks ();
    assert (0);
  case MODE_RENAME:
    rename_toks (tok_index);
    assert (0);
  case MODE_DELETE_STRING:
    delete_string (tok_index);
    assert (0);
  case MODE_RM_TOKS:
    rm_toks (tok_index);
    assert (0);
  case MODE_RM_TOK_PATTERN:
    rm_tok_pattern (tok_index);
    assert (0);
  }
}
