/* topformflat.l; see License.txt for copyright and terms of use
 * 
 * Scott McPeak        <smcpeak@cs.berkeley.edu>
 *
 * flatten all toplevel forms to single lines.
 * very heuristic... */

%{
#include <stdlib.h>     // atoi

// emit yytext as-is
void emit();

// debugging diagnostic, emitted when enabled
void diag(char const *str);

// add a newline if nesting <= threshold
void possibleNewline();

// keep track of brace nesting (0 means not inside any pair)
int nesting = 0;

// nesting threshold; when nesting is greater than threshold,
// newlines are suppressed
int threshold = 0;

%}

/* don't try to call yywrap() */
%option noyywrap
/* dsw: don't define yyunput() */
%option nounput

/* start condition for strings */
%x STRING
%x CHARLIT


%%

";"           { emit(); possibleNewline(); }

"/\n"         { if (nesting <= threshold) {    /* end of C comment */
                  emit();
                }
                else {
                  printf("%c", yytext[0]);
                }
              }

"{"           { nesting++;
                emit();
                possibleNewline();      // so the header is separated from the components
              }

"}"(";"?)     { nesting--;
                emit();
                possibleNewline();
              }

  /* a hash, then some non-newlines.  then, possibly, an escaped
   * newline followed by more non-newlines (repeat as needed).
   * finally, a newline */
"#".*("\\\n".*)*"\n" {
                printf("\n");      /* make sure starts on own line */
                emit();            /* preprocessor */
              }

"\n"          { printf(" "); }     /* not any above case, eat it*/

"//".*"\n"    { emit(); }          /* C++ comment */

"\""          { diag("<STR>"); emit(); BEGIN(STRING); }     /* start quote */

<STRING>{
  "\\"(.|\n)  { emit(); }                                   /* escaped character */
  "\""        { emit(); diag("</STR>"); BEGIN(INITIAL); }   /* close quote */
  (.|\n)      { emit(); }                                   /* ordinary char */
}

"\'"          { diag("<CHAR>"); emit(); BEGIN(CHARLIT); }   /* start tick */

<CHARLIT>{
  "\\"(.|\n)  { emit(); }                                   /* escaped character */
  "\'"        { emit(); diag("</CHAR>"); BEGIN(INITIAL); }  /* close tick */
  (.|\n)      { emit(); }                                   /* ordinary char */
}

.             { emit(); }

%%

void emit()
{
  printf("%.*s", yyleng, yytext);
}

void diag(char const *str)
{
  //printf("%s", str);
}

void possibleNewline()
{
  if (nesting <= threshold) {
    printf("\n");
  }
}

char *version = "2003.7.14";
int main(int argc, char *argv[])
{
  if (isatty(0)) {
    printf("topformflat version %s\n", version);
    printf("usage: %s [threshold] <input.c >output.c\n", argv[0]);
    printf("  The threshold (default: 0) specifies at what nesting level\n"
           "  of braces will line breaks be allowed (or inserted).  By\n"
           "  starting with 0, you get all top-level forms, one per line\n"
           "  (roughly).  Increasing the threshold leads to finer-grained\n"
           "  structure on each line.  The intent is to use the delta\n"
           "  minimizer on each level of granularity.\n");
    return 0;
  }

  if (argc >= 2) {
    threshold = atoi(argv[1]);    // user-specified threshold
  }  

  yyin = stdin;
  yylex();
  return 0;
}
