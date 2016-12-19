/*
 * Copyright (c) 2013, 2014, 2015, 2016 The University of Utah
 * All rights reserved.
 *
 * This file is distributed under the University of Illinois Open Source
 * License.  See the file COPYING for details.
 */

#if HAVE_CONFIG_H
#  include <config.h>
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

static struct tok_t *tok_list;
static int toks;
static int max_toks;
static const int initial_length = 1;

static int add_tok(char *str, enum tok_kind kind) {
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
  MODE_RM_TOKS,
  MODE_RM_TOK_PATTERN,
  MODE_SHORTEN_STRING,
  MODE_X_STRING,
  MODE_DEFINE,
  MODE_NONE,
};

static void print_toks(void) {
  int i;
  for (i = 0; i < toks; i++) {
    printf("%s", tok_list[i].str);
  }
  exit(OK);
}

static int next_char(char *c) {
  if (*c == 'z') {
    *c = 'a';
    return 1;
  }
  *c = 1 + *c;
  return 0;
}

static void next_name(char *name) {
  int pos = strlen(name) - 1;
  while (1) {
    int wrapped = next_char(&name[pos]);
    if (!wrapped)
      return;
    if (pos == 0) {
      // there's no next string at this length so prepend a character
      int i;
      int len = strlen(name);
      for (i = len; i >= 0; i--)
	name[i + 1] = name[i];
      name[0] = 'a';
      return;
    }
    pos--;
  }
}

static void find_unused_name(char *name) {
  strcpy(name, "a");
  int clash;
  do {
    clash = 0;
    int i;
    for (i = 0; i < toks; i++) {
      if (strcmp(tok_list[i].str, name) == 0) {
	next_name(name);
	clash = 1;
	break;
      }
    }
  } while (clash);
}

static int should_be_renamed(char *name, char *newname) {
  int i;
  for (i=0; i < strlen(name); i++) {
    if (name[i] < 'a' || name[i] > 'z')
      return 1;
  }
  if (strlen(newname) > strlen(name))
    return 0;
  return strcmp(newname, name) < 0;
}

static void index_toks(char ***index_ptr, int *index_size_ptr, char *newname) {
  char **index = 0;
  int index_size = 0;
  int i;
  for (i = 0; i < toks; i++) {
    if (tok_list[i].kind != TOK_IDENT)
      continue;
    if (!should_be_renamed(tok_list[i].str, newname))
      continue;
    int matched = 0;
    int j;
    for (j = 0; j < index_size; j++) {
      if (strcmp(index[j], tok_list[i].str) == 0) {
	matched = 1;
	tok_list[i].id = j;
	break;
      }
    }
    if (!matched) {
      tok_list[i].id = index_size;
      index = realloc(index, (1 + index_size) * sizeof(char *));
      index[index_size] = tok_list[i].str;
      index_size++;
    }
  }
  *index_ptr = index;
  *index_size_ptr = index_size;
}

static void print_renamed(int tok_index, char *newname) {
  int i;
  for (i = 0; i < toks; i++) {
    if (tok_list[i].id == tok_index)
      printf("%s", newname);
    else
      printf("%s", tok_list[i].str);
  }
}

static void rename_toks(int tok_index) {
  char newname[255];
  find_unused_name(newname);
  assert(tok_index >= 0);
  char **index;
  int index_size;
  index_toks(&index, &index_size, newname);
  //fprintf(stderr, "tok_index = %d, index size = %d\n", tok_index, index_size);
  if (tok_index >= index_size) {
    //fprintf(stderr, "rename_toks stop\n");
    exit(STOP);
  } else {
    //fprintf(stderr, "rename_toks with index %d, source '%s', target '%s'\n",
    // tok_index, index[tok_index], newname);
    print_renamed(tok_index, newname);
    exit(OK);
  }
}

static void string_rm_chars(char *s, int i) {
  int j;
  for (j = 0; j < (strlen(s) - i + 1); j++) {
    s[j] = s[j + i];
  }
}

static int remove_line(char *s, int idx, int *numlines) {
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

static void shorten_string(int idx) {
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

static void x_string(int idx) {
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

static void delete_string(int idx) {
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

static int n_toks;

static void rm_toks(int idx) {
  int i;
  int matched = 0;
  int which = 0;
  int started = 0;
  for (i = 0; i < toks; i++) {
    if (tok_list[i].kind != TOK_WS &&
        tok_list[i].kind != TOK_NEWLINE) {
      if (which == idx) {
        started = 1;
        matched = 1;
      }
      which++;
    }
    if (!started || (which > (idx + n_toks)))
      printf("%s", tok_list[i].str);
  }
  if (matched) {
    exit(OK);
  } else {
    exit(STOP);
  }
}

static void print_pattern(unsigned char c) {
  int z;
  for (z = 0; z < 8; z++) {
    printf("%d", (c & 128) ? 1 : 0);
    c <<= 1;
  }
  printf("\n");
}

static void rm_tok_pattern(int idx) {
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
    if (tok_list[i].kind != TOK_WS &&
        tok_list[i].kind != TOK_NEWLINE) {
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
    if (tok_list[i].kind == TOK_WS ||
        tok_list[i].kind == TOK_NEWLINE) {
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

// handle simple #defines
// todo: handle macro arguments
// todo: handle undefinition, redefinition, and other cases
// fixme: this is just extremely hacky-- partial preprocessing should be done by
// a separate tool that resembles unifdef
void replace_macro(int i) {
  int initial = i;
  char *macro = tok_list[i].str;
  // printf("replacing macro '%s'\n", macro);
  i++;
  while (tok_list[i].kind == TOK_WS)
    i++;
  int end = i;
  while (tok_list[end].kind != TOK_NEWLINE)
    end++;
  int x;
  for (x = 0; x < toks; ++x) {
    if (x != initial &&
        strcmp(tok_list[x].str, macro) == 0) {
      int y;
      for (y = i; y < end; ++y)
        printf("%s", tok_list[y].str);
    } else {
      printf("%s", tok_list[x].str);
    }
  }
}

void define(int tok_index) {
  int i;
  int found = 0;
  for (i = 0; i < toks; ++i) {
    if (strcmp(tok_list[i].str, "#") == 0) {
      i++;
      while (tok_list[i].kind == TOK_WS)
        i++;
      if (strcmp(tok_list[i].str, "define") != 0)
        continue;
      i++;
      while (tok_list[i].kind == TOK_WS)
        i++;
      int j;
      int used = 0;
      for (j = 0; j < toks; ++j)
        if (j != i &&
            strcmp(tok_list[j].str, tok_list[i].str) == 0)
          used = 1;
      if (!used)
        continue;
      if (found == tok_index) {
        replace_macro(i);
        exit(OK);
      }
      found++;
    }
  }
  exit(STOP);
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
  } else if (strcmp(cmd, "define") == 0) {
    mode = MODE_DEFINE;
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
  case MODE_RM_TOKS:
    rm_toks(tok_index);
    assert(0);
  case MODE_RM_TOK_PATTERN:
    rm_tok_pattern(tok_index);
    assert(0);
  case MODE_DEFINE:
    define(tok_index);
    assert(0);
  default:
    assert(0);
  }
}
