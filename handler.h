#ifndef _HANDLER_H
#define _HANDLER_H
int yylex();
int yyerror(char *s);
void yy_scan_string(const char *);
int yyparse();
void yylex_destroy();

int login(const char *name);
int logout();
int create_table(const char *name);
int join_table(const char *name);
int quit_table();
int ls(const char *name);
int pwd();
int join();
int raise(int);
int call();
int fold();
int check();
int reply(const char *fmt, ...);
#endif
