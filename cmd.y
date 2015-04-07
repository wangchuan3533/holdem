%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "handler.h"
%}

/* delcare token */
%token NUMBER 
%token ADD SUB MUL DIV OP CP
%token REG LOGIN LOGOUT SHOW TABLES PLAYERS IN MK JOIN PWD QUIT EXIT HELP
%token BET RAISE CALL CHECK FOLD CHAT PROMPT
%token IDENTIFIER STRING
%token EOL
%union
{
    int intval;
    char *strval;
}

%type <intval> exp factor term NUMBER
%type <strval> IDENTIFIER STRING

%%

calclist: /* empty */
  | calclist REG IDENTIFIER IDENTIFIER EOL         { reg($3, $4); free($3); free($4);      }
  | calclist LOGIN IDENTIFIER IDENTIFIER EOL       { login($3, $4); free($3); free($4);    }
  | calclist LOGOUT EOL                            { logout();                             }
  | calclist MK IDENTIFIER EOL                     { create_table($3); free($3);           }
  | calclist JOIN IDENTIFIER EOL                   { join_table($3); free($3);             }
  | calclist QUIT EOL                              { quit_table();                         }
  | calclist EXIT EOL                              { exit_game();                          }
  | calclist SHOW TABLES EOL                       { show_tables();                        }
  | calclist SHOW PLAYERS EOL                      { show_players();                       }
  | calclist SHOW PLAYERS IN IDENTIFIER EOL        { show_players_in_table($5); free($5);  }
  | calclist PWD EOL                               { pwd();                                }
  | calclist RAISE exp EOL                         { raise($3);                            }
  | calclist BET exp EOL                           { raise($3);                            }
  | calclist CALL EOL                              { call();                               }
  | calclist FOLD EOL                              { fold();                               }
  | calclist CHECK EOL                             { check();                              }
  | calclist HELP EOL                              { print_help();                         }
  | calclist CHAT STRING EOL                       { chat($3); free($3);                   }
  | calclist PROMPT STRING EOL                     { prompt($3); free($3);                 }
  | calclist exp EOL                               { reply("%d", $2);                      }
  | calclist EOL                                   { reply("\n");                          }
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
