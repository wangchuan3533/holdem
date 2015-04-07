%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "handler.h"
%}

/* delcare token */
%token NUMBER 
%token ADD SUB MUL DIV OP CP
%token LOGIN LOGOUT SHOW TABLES PLAYERS IN MK JOIN PWD EXIT HELP
%token RAISE CALL CHECK FOLD CHAT
%token SYMBOL STRING
%token EOL
%union
{
    int intval;
    char *strval;
}

%type <intval> exp factor term NUMBER
%type <strval> SYMBOL STRING

%%

calclist: /* empty */
  | calclist LOGIN SYMBOL EOL                      { login($3); free($3);              }
  | calclist LOGOUT EOL                            { logout();                         }
  | calclist MK SYMBOL EOL                         { create_table($3); free($3);       }
  | calclist JOIN SYMBOL EOL                       { join_table($3); free($3);         }
  | calclist EXIT EOL                              { quit_table();                     }
  | calclist SHOW TABLES EOL                       { show_tables();                    }
  | calclist SHOW PLAYERS EOL                      { show_players();                   }
  | calclist SHOW PLAYERS IN SYMBOL EOL            { show_players_in_table($5);        }
  | calclist PWD EOL                               { pwd();                            }
  | calclist RAISE exp EOL                         { raise($3);                        }
  | calclist CALL EOL                              { call();                           }
  | calclist FOLD EOL                              { fold();                           }
  | calclist CHECK EOL                             { check();                          }
  | calclist HELP EOL                              { print_help();                     }
  | calclist CHAT STRING EOL                       { chat($3);                         }
  | calclist exp EOL                               { reply("%d\ntexas> ", $2);         }
  | calclist EOL                                   { reply("\ntexas> ");               }
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
