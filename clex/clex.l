D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*

%option noyywrap

%{
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

void doit (void);
%}

%%

"auto"			{ doit(); }
"break"			{ doit(); }
"case"			{ doit(); }
"char"			{ doit(); }
"const"			{ doit(); }
"continue"		{ doit(); }
"default"		{ doit(); }
"do"			{ doit(); }
"double"		{ doit(); }
"else"			{ doit(); }
"enum"			{ doit(); }
"extern"		{ doit(); }
"float"			{ doit(); }
"for"			{ doit(); }
"goto"			{ doit(); }
"if"			{ doit(); }
"int"			{ doit(); }
"long"			{ doit(); }
"register"		{ doit(); }
"return"		{ doit(); }
"short"			{ doit(); }
"signed"		{ doit(); }
"sizeof"		{ doit(); }
"static"		{ doit(); }
"struct"		{ doit(); }
"switch"		{ doit(); }
"typedef"		{ doit(); }
"union"			{ doit(); }
"unsigned"		{ doit(); }
"void"			{ doit(); }
"volatile"		{ doit(); }
"while"			{ doit(); }

{L}({L}|{D})*		{ doit(); }

0[xX]{H}+{IS}?		{ doit(); }
0{D}+{IS}?		{ doit(); }
{D}+{IS}?		{ doit(); }
L?'(\\.|[^\\'])+'	{ doit(); }

{D}+{E}{FS}?		{ doit(); }
{D}*"."{D}+({E})?{FS}?	{ doit(); }
{D}+"."{D}*({E})?{FS}?	{ doit(); }

L?\"(\\.|[^\\"])*\"	{ doit(); }

"..."			{ doit(); }
">>="			{ doit(); }
"<<="			{ doit(); }
"+="			{ doit(); }
"-="			{ doit(); }
"*="			{ doit(); }
"/="			{ doit(); }
"%="			{ doit(); }
"&="			{ doit(); }
"^="			{ doit(); }
"|="			{ doit(); }
">>"			{ doit(); }
"<<"			{ doit(); }
"++"			{ doit(); }
"--"			{ doit(); }
"->"			{ doit(); }
"&&"			{ doit(); }
"||"			{ doit(); }
"<="			{ doit(); }
">="			{ doit(); }
"=="			{ doit(); }
"!="			{ doit(); }
";"			{ doit(); }
("{"|"<%")		{ doit(); }
("}"|"%>")		{ doit(); }
","			{ doit(); }
":"			{ doit(); }
"="			{ doit(); }
"("			{ doit(); }
")"			{ doit(); }
("["|"<:")		{ doit(); }
("]"|":>")		{ doit(); }
"."			{ doit(); }
"&"			{ doit(); }
"!"			{ doit(); }
"~"			{ doit(); }
"-"			{ doit(); }
"+"			{ doit(); }
"*"			{ doit(); }
"/"			{ doit(); }
"%"			{ doit(); }
"<"			{ doit(); }
">"			{ doit(); }
"^"			{ doit(); }
"|"			{ doit(); }
"?"			{ doit(); }
"#"                     { doit(); }
"\\"                    { doit(); }

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

[ \t\v\n\f]		{  }

.			{ fprintf (stderr, "didn't expect to see '%s'\n", yytext); 
                          assert (0); }

%%

int count = 0;

