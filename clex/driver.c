/*
 * Copyright (c) 2013, 2014, 2015, 2016 The University of Utah
 * All rights reserved.
 *
 * This file is distributed under the University of Illinois Open Source
 * License.  See the file COPYING for details.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"

struct tok_t {
  char *str;
  enum tok_kind kind;
  int id;
};

struct tok_t *tok_list;
int toks;
int max_toks;
const int initial_length = 1;

int add_tok(char *str, enum tok_kind kind) {
  assert(str);
  if (toks >= max_toks) {
    max_toks *= 2;
    tok_list =
        (struct tok_t *)realloc(tok_list, max_toks * sizeof(struct tok_t));
    assert(tok_list);
  }
  tok_list[toks].str = strdup(str);
  assert(tok_list[toks].str);
  tok_list[toks].kind = kind;
  tok_list[toks].id = -1;
  toks++;
  return toks - 1;
}

void process_token(enum tok_kind kind) {
  int tok = add_tok(yytext, kind);
  count++;
}

enum mode_t {
  MODE_RENAME = 1111,
  MODE_PRINT,
  MODE_DELETE_STRING,
  MODE_REVERSE_TOKS,
  MODE_RM_TOKS,
  MODE_RM_TOK_PATTERN,
  MODE_COLLAPSE_TOKS,
  MODE_SHORTEN_STRING,
  MODE_X_STRING,
  MODE_SHORTEN_INT,
  MODE_REMOVE_ASM_COMMENT,
  MODE_REMOVE_ASM_LINE,
  MODE_NONE,
};

void print_toks(void) {
  int i;
  for (i = 0; i < toks; i++) {
    printf("%s", tok_list[i].str);
  }
  exit(OK);
}

void number_tokens(int ignore_renamed, int *max_id_seen_p, int *max_tok_p) {
  int next_id = 0;
  int max_id_seen = -1;
  int i;
  for (i = 0; i < toks; i++) {
    if (tok_list[i].kind != TOK_IDENT)
      continue;
    int id;
    int res = sscanf(tok_list[i].str, "x%d", &id);
    if (res == 1) {
      if (id > max_id_seen) {
        max_id_seen = id;
      }
      if (ignore_renamed)
        continue;
    }
    int j;
    int matched = 0;
    for (j = 0; j < i; j++) {
      if (tok_list[j].kind != TOK_IDENT)
        continue;
      if (strcmp(tok_list[j].str, tok_list[i].str) == 0) {
        matched = 1;
        tok_list[i].id = tok_list[j].id;
        assert(tok_list[j].id != -1);
      }
    }
    if (!matched) {
      tok_list[i].id = next_id;
      next_id++;
    }
  }
  // FIXME find first unused instead of max_id_seen?
  if (max_id_seen_p)
    *max_id_seen_p = max_id_seen;
  if (max_tok_p)
    *max_tok_p = next_id;
}

void collapse_toks(int tok_index) {
  assert(tok_index >= 0);
  int max_tok_id;
  number_tokens(0, NULL, &max_tok_id);
  // fprintf (stderr, "tok_index = %d, number of tokens = %d, max_tok_id =
  // %d\n", tok_index, toks, max_tok_id);
  int counter = -1;
  int i;
  for (i = 0; i < max_tok_id; i++) {
    int j;
    for (j = 0; j < max_tok_id; j++) {
      if (i == j)
        continue;
      counter++;
      if (counter == tok_index) {
        // rename i to have the same name as j
        int k;
        char *new_name = NULL;
        for (k = 0; k < toks; k++) {
          if (tok_list[k].id == j) {
            new_name = tok_list[k].str;
          }
        }
        assert(new_name);
        for (k = 0; k < toks; k++) {
          if (tok_list[k].id == i) {
            // fprintf (stderr, "renaming '%s' to '%s'\n", tok_list[i].str,
            // new_name);
            printf("%s", new_name);
          } else {
            printf("%s", tok_list[k].str);
          }
        }
        exit(OK);
      }
    }
  }
  exit(STOP);
}

// FIXME: have a C++ mode that avoids trying to rename C++ keywords?

void rename_toks(int tok_index) {
  assert(tok_index >= 0);
  int unused;
  number_tokens(1, &unused, NULL);
  char newname[255];
  sprintf(newname, "x%d", unused + 1);
  int matched = 0;
  char *oldname = NULL;
  int i;
  // dump the renamed token stream
  for (i = 0; i < toks; i++) {
    if (tok_list[i].id == tok_index) {
      assert(!oldname || strcmp(oldname, tok_list[i].str) == 0);
      oldname = tok_list[i].str;
      matched = 1;
      printf("%s", newname);
    } else {
      printf("%s", tok_list[i].str);
    }
  }
  if (matched) {
    // printf ("/* we renamed '%s' to '%s' */\n", oldname, newname);
    exit(OK);
  } else {
    exit(STOP);
  }
}

void string_rm_chars(char *s, int i) {
  int j;
  for (j = 0; j < (strlen(s) - i + 1); j++) {
    s[j] = s[j + i];
  }
}

void delete_asm_comment(char *s) {
  int i;
  for (i = 0; s[i] != '\\'; i++) {
  }
  string_rm_chars(s, i);
}

void remove_asm_comment(int idx) {
  int i;
  int matched = 0;
  int which = 0;
  for (i = 0; i < toks; i++) {
    if (tok_list[i].kind == TOK_STRING) {
      int j = 0;
      char *s = tok_list[i].str;
      while (s[j] != 0) {
        if (s[j] == '#') {
          if (idx == which) {
            matched = 1;
            delete_asm_comment(&s[j]);
            break;
          }
          which++;
        }
        j++;
      }
    }
    printf("%s", tok_list[i].str);
  }
  if (matched) {
    exit(OK);
  } else {
    exit(STOP);
  }
}

int remove_line(char *s, int idx, int *numlines) {
  int line = 0;
  int lastpos = 1;
  int ret = 0;
  int i;
  for (i = 0; i < strlen(s); i++) {
    if (strncmp(s + i, "\\n", 2) == 0 || s[i + 1] == 0) {
      if (line == idx) {
        if (s[i + 1] == 0) {
          // printf ("removing rest of string at %d\n", lastpos);
          s[lastpos] = '"';
          s[lastpos + 1] = 0;
        } else {
          // printf ("removing %d chars at %d\n", i - lastpos + 2, lastpos);
          string_rm_chars(s + lastpos, i - lastpos + 2);
          ret = 1;
        }
      }
      lastpos = i + 2;
      line++;
    }
  }
  *numlines = line;
  return ret;
}

void remove_asm_line(int idx) {
  int i;
  int matched = 0;
  for (i = 0; i < toks; i++) {
    if (!matched && tok_list[i].kind == TOK_STRING) {
      char *s = tok_list[i].str;
      int numlines;
      int res = remove_line(s, idx, &numlines);
      if (res) {
        matched = 1;
      } else {
        idx -= numlines;
      }
    }
    printf("%s", tok_list[i].str);
  }
  if (matched) {
    exit(OK);
  } else {
    exit(STOP);
  }
}

void shorten_string(int idx) {
  int i;
  int matched = 0;
  int which = 0;
  for (i = 0; i < toks; i++) {
    if (!matched && tok_list[i].kind == TOK_STRING) {
      char *s = tok_list[i].str;
      int len = strlen(s) - 2;
      if (idx >= len) {
        idx -= len;
      } else {
        string_rm_chars(s + idx + 1, 1);
        matched = 1;
        which++;
      }
    }
    printf("%s", tok_list[i].str);
  }
  if (matched) {
    exit(OK);
  } else {
    exit(STOP);
  }
}

void x_string(int idx) {
  int i;
  int matched = 0;
  int which = 0;
  for (i = 0; i < toks; i++) {
    if (!matched && tok_list[i].kind == TOK_STRING) {
      char *s = tok_list[i].str;
      int j;
      for (j = 0; j < strlen(s); j++) {
        if (s[j] != 'x') {
          if (which == idx) {
            s[j] = 'x';
            matched = 1;
          }
          which++;
        }
      }
    }
    printf("%s", tok_list[i].str);
  }
  if (matched) {
    exit(OK);
  } else {
    exit(STOP);
  }
}

int nonzero_digit(char c) { return (c >= '1' && c <= '9'); }

int count_nonzero(char *s) {
  int c = 0;
  while (*s != 0) {
    if (nonzero_digit(*s))
      c++;
    s++;
  }
  return c;
}

void shorten_int(int idx) {
  int i;
  int matched = 0;
  for (i = 0; i < toks; i++) {
    if (!matched && tok_list[i].kind == TOK_NUMBER) {
      char *s = tok_list[i].str;
      int l = count_nonzero(s);
      if (idx > l) {
        idx -= l;
      } else {
      }
    }
    printf("%s", tok_list[i].str);
  }
  if (matched) {
    exit(OK);
  } else {
    exit(STOP);
  }
}

void delete_string(int idx) {
  int i;
  int matched = 0;
  int which = 0;
  for (i = 0; i < toks; i++) {
    int printed = 0;
    if (tok_list[i].kind == TOK_STRING &&
        strcmp(tok_list[i].str, "\"\"") != 0) {
      if (which == idx) {
        printf("\"\"");
        printed = 1;
        matched = 1;
      }
      which++;
    }
    if (!printed)
      printf("%s", tok_list[i].str);
  }
  if (matched) {
    exit(OK);
  } else {
    exit(STOP);
  }
}

int n_toks;

void reverse_toks(int idx) {
  const int N = 128;
  int matched = 0;
  int which = 0;
  int i;

#ifdef _MSC_VER
  int *saved = calloc(N, sizeof(int));
#else
  int saved[N];
#endif

  int nsaved = 0;
  for (i = 0; i < toks; i++) {
    if (which >= idx && which < idx + n_toks) {
      saved[nsaved] = i;
      nsaved++;
    } else {
      printf("%s", tok_list[i].str);
    }
    if (which == idx + n_toks) {
      int x;
      for (x = nsaved - 1; x >= 0; x--) {
        printf("%s", tok_list[saved[x]].str);
      }
    }
    if (tok_list[i].kind != TOK_WS) {
      which++;
    }
  }

#ifdef _MSC_VER
  free(saved);
#endif

  if (matched) {
    exit(OK);
  } else {
    exit(STOP);
  }
}

void rm_toks(int idx) {
  int i;
  int matched = 0;
  int which = 0;
  int started = 0;
  for (i = 0; i < toks; i++) {
    if (tok_list[i].kind != TOK_WS) {
      if (which == idx) {
        started = 1;
        matched = 1;
      }
      which++;
    }
    if (!started || (started && which > (idx + n_toks)))
      printf("%s", tok_list[i].str);
  }
  if (matched) {
    exit(OK);
  } else {
    exit(STOP);
  }
}

void print_pattern(unsigned char c) {
  int z;
  for (z = 0; z < 8; z++) {
    printf("%d", (c & 128) ? 1 : 0);
    c <<= 1;
  }
  printf("\n");
}

void rm_tok_pattern(int idx) {
  int i;
  int n_patterns = 1 << (n_toks - 1);

#ifdef _MSC_VER
  unsigned char *patterns = calloc(n_patterns, sizeof(unsigned char));
#else
  unsigned char patterns[n_patterns];
#endif

  for (i = 0; i < n_patterns; i++) {
    patterns[i] = 1 | ((unsigned)i << 1);
  }

  int n_pattern = idx & (n_patterns - 1);
  unsigned char pat = patterns[n_pattern];

#ifdef _MSC_VER
  free(patterns);
#endif

  if (0) {
    printf("pattern = ");
    print_pattern(pat);
  }

  idx >>= (n_toks - 1);

  int which = 0;
  int started = 0;
  int matched = 0;
  int deleted = 0;
  for (i = 0; i < toks; i++) {
    if (tok_list[i].kind != TOK_WS) {
      if (which == idx) {
        matched = 1;
        started = 1;
      }
      if (which == (idx + n_toks))
        started = 0;
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
    if (print)
      printf("%s", tok_list[i].str);
  }
  if (matched && deleted) {
    exit(OK);
  } else {
    exit(STOP);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("USAGE: %s command index file\n", argv[0]);
    exit(STOP);
  }

  char *cmd = argv[1];
  enum mode_t mode = MODE_NONE;
  if (strcmp(cmd, "rename-toks") == 0) {
    mode = MODE_RENAME;
  } else if (strcmp(cmd, "print") == 0) {
    mode = MODE_PRINT;
  } else if (strcmp(cmd, "delete-string") == 0) {
    mode = MODE_DELETE_STRING;
  } else if (strcmp(cmd, "shorten-string") == 0) {
    mode = MODE_SHORTEN_STRING;
  } else if (strcmp(cmd, "x-string") == 0) {
    mode = MODE_X_STRING;
  } else if (strcmp(cmd, "shorten-int") == 0) {
    mode = MODE_SHORTEN_INT;
  } else if (strcmp(cmd, "remove-asm-comment") == 0) {
    mode = MODE_REMOVE_ASM_COMMENT;
  } else if (strcmp(cmd, "remove-asm-line") == 0) {
    mode = MODE_REMOVE_ASM_LINE;
  } else if (strcmp(cmd, "collapse-toks") == 0) {
    mode = MODE_COLLAPSE_TOKS;
  } else if (strncmp(cmd, "reverse-", 8) == 0) {
    mode = MODE_REVERSE_TOKS;
    int res = sscanf(&cmd[8], "%d", &n_toks);
    assert(res == 1);
    assert(n_toks > 0 && n_toks <= 1000);
  } else if (strncmp(cmd, "rm-toks-", 8) == 0) {
    mode = MODE_RM_TOKS;
    int res = sscanf(&cmd[8], "%d", &n_toks);
    assert(res == 1);
    assert(n_toks > 0 && n_toks <= 1000);
  } else if (strncmp(cmd, "rm-tok-pattern-", 15) == 0) {
    mode = MODE_RM_TOK_PATTERN;
    int res = sscanf(&cmd[15], "%d", &n_toks);
    assert(res == 1);
    assert(n_toks > 1 && n_toks <= 8);
  } else {
    printf("error: unknown mode '%s'\n", cmd);
    assert(0);
  }

  int tok_index;
  int ret = sscanf(argv[2], "%d", &tok_index);
  assert(ret == 1);
  // printf ("file = '%s'\n", argv[3]);
  FILE *in = fopen(argv[3], "r");
  assert(in);
  yyin = in;

  max_toks = initial_length;
  tok_list = (struct tok_t *)malloc(max_toks * sizeof(struct tok_t));
  assert(tok_list);

  yylex();

  // these calls all exit() at the end
  switch (mode) {
  case MODE_PRINT:
    print_toks();
    assert(0);
  case MODE_RENAME:
    rename_toks(tok_index);
    assert(0);
  case MODE_DELETE_STRING:
    delete_string(tok_index);
    assert(0);
  case MODE_SHORTEN_STRING:
    shorten_string(tok_index);
    assert(0);
  case MODE_X_STRING:
    x_string(tok_index);
    assert(0);
  case MODE_SHORTEN_INT:
    shorten_int(tok_index);
    assert(0);
  case MODE_REMOVE_ASM_COMMENT:
    remove_asm_comment(tok_index);
    assert(0);
  case MODE_REMOVE_ASM_LINE:
    remove_asm_line(tok_index);
    assert(0);
  case MODE_COLLAPSE_TOKS:
    collapse_toks(tok_index);
    assert(0);
  case MODE_RM_TOKS:
    rm_toks(tok_index);
    assert(0);
  case MODE_REVERSE_TOKS:
    reverse_toks(tok_index);
    assert(0);
  case MODE_RM_TOK_PATTERN:
    rm_tok_pattern(tok_index);
    assert(0);
  default:
    assert(0);
  }
}
