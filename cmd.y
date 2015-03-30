%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int yylex();
int yyerror(char *s);
%}

/* delcare token */
%token NUMBER
%token ADD SUB MUL DIV
%token OP CP
%token LS CD PWD EXIT
%token RAISE CHECK FOLD
%token EOL

%%

calclist: /* empty */
  | calclist LS EOL { printf("ls\n"); }
  | calclist CD exp EOL { printf("cd %d\n", $3); }
  | calclist PWD EOL { printf("pwd\n"); }
  | calclist EXIT EOL { exit(0); }
  | calclist RAISE exp EOL { printf("raise %d\n", $3); }
  | calclist FOLD EOL { printf("fold\n"); }
  | calclist CHECK EOL { printf("check\n"); }
  | calclist exp EOL { printf("= %d\n", $2); }
  | calclist EOL { printf(">"); }
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

int yyerror(char *s)
{
    fprintf(stderr, "error: %s\n", s);
    return 0;
}
