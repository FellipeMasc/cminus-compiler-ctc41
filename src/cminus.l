/****************************************************/
/* File: cminus.l                                   */
/* Lex specification for C-                         */
/* Project for CES41: Compiladores                  */
/****************************************************/

%option noyywrap 
/* opção noyywrap pode ser necessária para novas versões do flex
  limitação: não compila mais de um arquivo fonte de uma só vez (não precisamos disso)
  https://stackoverflow.com/questions/1480138/undefined-reference-to-yylex 
*/ 

%{
#define YYSTYPE TreeNode *
#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parser.h"
/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN+1];
char prevTokenString[MAXTOKENLEN+1];
char currentLineBuffer[256];
%}


digit       [0-9]
number      {digit}+
letter      [a-zA-Z]
identifier  {letter}+
newline     \r?\n
whitespace  [ \t]+


%%
"if"            {return IF;}
"then"          {return THEN;}
"else"          {return ELSE;}
"end"           {return END;}
"repeat"        {return REPEAT;}
"until"         {return UNTIL;}
"read"          {return READ;}
"write"         {return WRITE;}
"void"          {return VOID;}
"int"           {return INT;}
"while"         {return WHILE;}
"return"        {return RETURN;}
":="            {return ASSIGN;}
"="             {return EQ;}
"=="            {return EQQ;}
"!="            {return NEQ;}
"<"             {return LT;}
">"             {return GT;}
"<="            {return LTE;}
">="            {return GTE;}
"+"             {return PLUS;}
"-"             {return MINUS;}
"*"             {return TIMES;}
"/"             {return OVER;}
"("             {return LPAREN;}
")"             {return RPAREN;}
";"             {return SEMI;}
","             {return COMMA;}
{number}        {return NUM;}
{identifier}    {return ID;}
{newline}       {lineno++; printLine(redundant_source);}
{whitespace}    {/* skip whitespace */}
"{"             {return LBRACE;}
"}"             {return RBRACE;}
"["             {return LBRACKET;}
"]"             {return RBRACKET;}
<<EOF>>             {yyterminate();}
"/*"            {
                  char c;
                  do {
                      c = input();
                      if (c == EOF) break;
                      if (c == '\n') {lineno++; printLine(redundant_source);}
                  } while (c != '*' || input() != '/');
                }

.               {return ERROR;}




%%
TokenType getToken(void)
{ 
  static int firstTime = TRUE;
  TokenType currentToken;
  if (firstTime)
  { firstTime = FALSE;
    lineno++;
    yyin = source;
    yyout = listing;
  }
  if(FirstLine) {
    printLine(redundant_source);
    FirstLine = FALSE;
  }
  strncpy(prevTokenString,tokenString,MAXTOKENLEN);
  currentToken = yylex();
  strncpy(tokenString,yytext,MAXTOKENLEN);
  if (TraceScan) {
    pc("\t%d: ",lineno);
    printToken(currentToken,tokenString);
  }
  return currentToken;
}
