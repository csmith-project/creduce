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

enum tok_kind {
  TOK_KEYWORD = 999,
  TOK_OP,
  TOK_IDENT,
  TOK_OTHER,
  TOK_NUMBER,
  TOK_WS,
  TOK_STRING,
};

void process_token (enum tok_kind);
