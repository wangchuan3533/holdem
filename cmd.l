%{
#include "cmd.tab.h"
#include <string.h>
%}
%option noinput
%option nounput
%%
"login"                     { return LOGIN;                                             }
"logout"                    { return LOGOUT;                                            }
"mk"                        { return MK;                                                }
"join"                      { return JOIN;                                              }
"exit"                      { return EXIT;                                              }
"show"                      { return SHOW;                                              }
"tables"                    { return TABLES;                                            }
"players"                   { return PLAYERS;                                           }
"in"                        { return IN;                                                }
"pwd"                       { return PWD;                                               }
"raise"                     { return RAISE;                                             }
"call"                      { return CALL;                                              }
"fold"                      { return FOLD;                                              }
"check"                     { return CHECK;                                             }
"yell"                      { return YELL;                                              }
"help"                      { return HELP;                                              }
"+"                         { return ADD;                                               }
"-"                         { return SUB;                                               }
"*"                         { return MUL;                                               }
"/"                         { return DIV;                                               }
"("                         { return OP;                                                }
")"                         { return CP;                                                }
\n                          { return EOL;                                               }
[0-9]+                      { yylval.intval = atoi(yytext); return NUMBER;              }
0x[a-f0-9]+                 { yylval.intval = strtol(yytext, NULL, 16); return NUMBER;  }
[a-zA-Z][a-zA-Z0-9]*        { yylval.strval = strdup(yytext); return SYMBOL;            }
.
%%
