%{
#include "cmd.tab.h"
#include <string.h>
%}
%option noinput
%option nounput
%%
"reg"                       { return REG;                                                                   }
"login"                     { return LOGIN;                                                                 }
"logout"                    { return LOGOUT;                                                                }
"mk"                        { return MK;                                                                    }
"join"                      { return JOIN;                                                                  }
"quit"                      { return QUIT;                                                                  }
"exit"                      { return EXIT;                                                                  }
"show"                      { return SHOW;                                                                  }
"tables"                    { return TABLES;                                                                }
"players"                   { return PLAYERS;                                                               }
"in"                        { return IN;                                                                    }
"pwd"                       { return PWD;                                                                   }
"start"                     { return START;                                                                 }
"bet"                       { return BET;                                                                   }
"raise"                     { return RAISE;                                                                 }
"call"                      { return CALL;                                                                  }
"fold"                      { return FOLD;                                                                  }
"check"                     { return CHECK;                                                                 }
"all in"                    { return ALL_IN;                                                                }
"help"                      { return HELP;                                                                  }
"chat"                      { return CHAT;                                                                  }
"prompt"                    { return PROMPT;                                                                }
"+"                         { return ADD;                                                                   }
"-"                         { return SUB;                                                                   }
"*"                         { return MUL;                                                                   }
"/"                         { return DIV;                                                                   }
"("                         { return OP;                                                                    }
")"                         { return CP;                                                                    }
\n                          { return EOL;                                                                   }
[0-9]+                      { yylval.intval = atoi(yytext); return NUMBER;                                  }
0x[a-f0-9]+                 { yylval.intval = strtol(yytext, NULL, 16); return NUMBER;                      }
[a-zA-Z0-9]+                { yylval.strval = strdup(yytext); return IDENTIFIER;                            }
'[^']*'                     { yylval.strval = strndup(yytext + 1, strlen(yytext) - 2); return STRING;       }
.
%%
