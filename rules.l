/*****************************************************************************
 *   rules.l - Flex Rules for TIPS Language Subset
 *   MSU CSE 4714/6714 Capstone Project (Fall 2025)
 *   Author: Derek Willis
*****************************************************************************/
%option noyywrap nodefault nounput
%option yylineno

%{
#include "lexer.h"
#include <algorithm>
#include <string>
%}

%%
PROGRAM          { return PROGRAM; }
BEGIN            { return TOK_BEGIN; }
END              { return END; }
WRITE            { return WRITE; }
VAR              { return VAR; }
INTEGER          { return INTEGER; }
REAL             { return REAL; }
READ             { return READ; }
IF               { return IF; }
THEN             { return THEN; }
ELSE             { return ELSE; }
WHILE            { return WHILE; }
SENIORITIS       { return CUSTOM; }
AND              { return TOK_AND; }
OR               { return TOK_OR; }
NOT              { return TOK_NOT; }
CUSTOM           { return CUSTOM; }
MOD              { return MOD; }

##[^\n]*         { /* skip comments */ }

[A-Z][A-Z0-9]{8,} { return UNKNOWN; }
[A-Z][A-Z0-9]{0,7} { return IDENT; }

\'[^'\n]*\' {
    std::string lexeme(yytext, yyleng);
    size_t quoteCount = std::count(lexeme.begin(), lexeme.end(), '\'');
    if (quoteCount != 2) { return UNKNOWN; }
    std::string content = lexeme.substr(1, lexeme.size() - 2);
    if (content.size() > 80) { return UNKNOWN; }
    return STRINGLIT;
}
\'[^'\n]* { return UNKNOWN; }

\(              { return OPENPAREN; }
\)              { return CLOSEPAREN; }
:=              { return ASSIGN; }
:               { return COLON; }
;               { return SEMICOLON; }
"++"            { return INCREMENT; }
"--"            { return DECREMENT; }
"+"             { return PLUS; }
"-"             { return MINUS; }
"*"             { return MULTIPLY; }
"/"             { return DIVIDE; }
"^^"            { return CUSTOM_OPER; }
"="             { return EQUALTO; }
"<>"            { return NOTEQUALTO; }
"<"             { return LESSTHAN; }
">"             { return GREATERTHAN; }
[0-9]+\.[0-9]+  { return FLOATLIT; }
[0-9]+          { return INTLIT; }

[ \t\r\n]+      { /* skip whitespace */ }

.               { return UNKNOWN; }


%%
