%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "handler.h"
%}

/* delcare token */
%token NUMBER 
%token ADD SUB MUL DIV OP CP
%token LOGIN LOGOUT LS MK JOIN PWD EXIT
%token RAISE CALL CHECK FOLD YELL
%token SYMBOL
%token EOL
%union
{
    int intval;
    char *strval;
}

%type <intval> exp factor term NUMBER
%type <strval> SYMBOL

%%

calclist: /* empty */
  | calclist LOGIN SYMBOL EOL                      { login($3); free($3); }
  | calclist LOGOUT EOL                            { logout(); }
  | calclist MK SYMBOL EOL                         { create_table($3); free($3);}
  | calclist JOIN SYMBOL EOL                       { join_table($3); free($3);}
  | calclist EXIT EOL                              { quit_table(); }
  | calclist LS SYMBOL EOL                         { ls($3); free($3);}
  | calclist LS EOL                                { ls(NULL); }
  | calclist PWD EOL                               { pwd(); }
  | calclist RAISE exp EOL                         { raise($3); }
  | calclist CALL EOL                              { call(); }
  | calclist FOLD EOL                              { fold(); }
  | calclist CHECK EOL                             { check(); }
  | calclist exp EOL                               { reply("%d\ntexas> ", $2); }
  | calclist EOL                                   { reply("\ntexas> "); }
  ;

exp: factor
  | exp ADD factor { $$ = $1 + $3; }
  | exp SUB factor { $$ = $1 - $3; }
  ;

factor: term
  | factor MUL term { $$ = $1 * $3; }
  | factor DIV term { $$ = $1 / $3; }
  ;

term: NUMBER
  | OP exp CP    { $$ = $2; }
  ;

%%
//int main(int argc, char **argv)
//{
    //yy_scan_string(line);
//    yyparse();
    //yylex_destroy();
//}
