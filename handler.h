#ifndef _HANDLER_H
#define _HANDLER_H
int yylex();
int yyerror(char *s);
void yy_scan_string(const char *);
int yyparse();
void yylex_destroy();

int reg(const char *name, const char *password);
int login(const char *name, const char *password);
int logout();
int create_table(const char *name);
int join_table(const char *name);
int quit_table();
int exit_game();
int show_tables();
int show_players();
int show_players_in_table(const char *name);
int pwd();
int join();
int bet(int);
int raise_(int);
int call();
int fold();
int check();
int all_in();
int print_help();
int reply(const char *fmt, ...);
int chat(const char *str);
int prompt(const char *str);
#endif
