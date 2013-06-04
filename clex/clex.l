D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*

%option noyywrap

%{

#include <stuff.h>

%}

%%

"auto"			{ doit(TOK_KEYWORD); }
"break"			{ doit(TOK_KEYWORD); }
"case"			{ doit(TOK_KEYWORD); }
"char"			{ doit(TOK_KEYWORD); }
"const"			{ doit(TOK_KEYWORD); }
"continue"		{ doit(TOK_KEYWORD); }
"default"		{ doit(TOK_KEYWORD); }
"do"			{ doit(TOK_KEYWORD); }
"double"		{ doit(TOK_KEYWORD); }
"else"			{ doit(TOK_KEYWORD); }
"enum"			{ doit(TOK_KEYWORD); }
"extern"		{ doit(TOK_KEYWORD); }
"float"			{ doit(TOK_KEYWORD); }
"for"			{ doit(TOK_KEYWORD); }
"goto"			{ doit(TOK_KEYWORD); }
"if"			{ doit(TOK_KEYWORD); }
"int"			{ doit(TOK_KEYWORD); }
"long"			{ doit(TOK_KEYWORD); }
"register"		{ doit(TOK_KEYWORD); }
"return"		{ doit(TOK_KEYWORD); }
"short"			{ doit(TOK_KEYWORD); }
"signed"		{ doit(TOK_KEYWORD); }
"sizeof"		{ doit(TOK_KEYWORD); }
"static"		{ doit(TOK_KEYWORD); }
"struct"		{ doit(TOK_KEYWORD); }
"switch"		{ doit(TOK_KEYWORD); }
"typedef"		{ doit(TOK_KEYWORD); }
"union"			{ doit(TOK_KEYWORD); }
"unsigned"		{ doit(TOK_KEYWORD); }
"void"			{ doit(TOK_KEYWORD); }
"volatile"		{ doit(TOK_KEYWORD); }
"while"			{ doit(TOK_KEYWORD); }

{L}({L}|{D})*		{ doit(TOK_IDENT); }

0[xX]{H}+{IS}?		{ doit(TOK_NUMBER); }
0{D}+{IS}?		{ doit(TOK_NUMBER); }
{D}+{IS}?		{ doit(TOK_NUMBER); }
L?'(\\.|[^\\'])+'	{ doit(TOK_OTHER); }

{D}+{E}{FS}?		{ doit(TOK_NUMBER); }
{D}*"."{D}+({E})?{FS}?	{ doit(TOK_NUMBER); }
{D}+"."{D}*({E})?{FS}?	{ doit(TOK_NUMBER); }

L?\"(\\.|[^\\"])*\"	{ doit(TOK_OTHER); }

"..."			{ doit(TOK_OTHER); }
">>="			{ doit(TOK_OP); }
"<<="			{ doit(TOK_OP); }
"+="			{ doit(TOK_OP); }
"-="			{ doit(TOK_OP); }
"*="			{ doit(TOK_OP); }
"/="			{ doit(TOK_OP); }
"%="			{ doit(TOK_OP); }
"&="			{ doit(TOK_OP); }
"^="			{ doit(TOK_OP); }
"|="			{ doit(TOK_OP); }
">>"			{ doit(TOK_OP); }
"<<"			{ doit(TOK_OP); }
"++"			{ doit(TOK_OP); }
"--"			{ doit(TOK_OP); }
"->"			{ doit(TOK_OP); }
"&&"			{ doit(TOK_OP); }
"||"			{ doit(TOK_OP); }
"<="			{ doit(TOK_OP); }
">="			{ doit(TOK_OP); }
"=="			{ doit(TOK_OP); }
"!="			{ doit(TOK_OP); }
";"			{ doit(TOK_OP); }
("{"|"<%")		{ doit(TOK_OP); }
("}"|"%>")		{ doit(TOK_OP); }
","			{ doit(TOK_OP); }
":"			{ doit(TOK_OP); }
"="			{ doit(TOK_OP); }
"("			{ doit(TOK_OP); }
")"			{ doit(TOK_OP); }
("["|"<:")		{ doit(TOK_OP); }
("]"|":>")		{ doit(TOK_OP); }
"."			{ doit(TOK_OP); }
"&"			{ doit(TOK_OP); }
"!"			{ doit(TOK_OP); }
"~"			{ doit(TOK_OP); }
"-"			{ doit(TOK_OP); }
"+"			{ doit(TOK_OP); }
"*"			{ doit(TOK_OP); }
"/"			{ doit(TOK_OP); }
"%"			{ doit(TOK_OP); }
"<"			{ doit(TOK_OP); }
">"			{ doit(TOK_OP); }
"^"			{ doit(TOK_OP); }
"|"			{ doit(TOK_OP); }
"?"			{ doit(TOK_OP); }
"#"                     { doit(TOK_OTHER); }
"\\"                    { doit(TOK_OTHER); }

"/*"        {
                     for ( ; ; )  {
                         int c;
                         while ( (c = input()) != '*' &&
                                  c != EOF )
                             ;    /* eat up text of comment */
     
                         if ( c == '*' )
                             {
                             while ( (c = input()) == '*' )
                                 ;
                             if ( c == '/' )
                                 break;    /* found the end */
                             }
     
                         if ( c == EOF )
                             {
                             fprintf(stderr, "EOF in comment" );
			     assert (0);
                             }
                    }
           }

[ \t\v\n\f]		{ doit(TOK_WS); }

.			{ fprintf (stderr, "didn't expect to see '%s'\n", yytext); 
                          assert (0); }

%%

int count = 0;

