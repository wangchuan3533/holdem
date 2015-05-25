CC=gcc
LEX=flex
YACC=bison
RM=rm -rf
C_FLAGS=-g -Wall
#C_FLAGS+=-Werror
C_FLAGS+=-DTEXAS_ASSERT
#LD_FLAGS=-levent -lfl -ldb
LD_FLAGS=-levent -ll -ldb
LEX_FILES=$(wildcard *.l)
YACC_FILES=$(wildcard *.y)
HEADERS=$(wildcard *.h) $(YACC_FILES:.y=.tab.h)
SOURCES=$(wildcard *.c) $(YACC_FILES:.y=.tab.c) lex.yy.c
OBJS=$(SOURCES:.c=.o)

.PHONY: clean
all: server

unit_test: C_FLAGS+=-DUNIT_TEST

server : $(OBJS)
	$(CC) $^ -o $@ $(LD_FLAGS)
unit_test : $(OBJS)
	$(CC) $^ -o $@ $(LD_FLAGS)
%.o: %.c
	$(CC) $(C_FLAGS) -c $< -o $@
%.tab.o: %.tab.c
	$(CC) $(C_FLAGS) -c $< -o $@
%.yy.o: %.yy.c
	$(CC) $(C_FLAGS) -c $< -o $@
lex.yy.c : $(LEX_FILES)
	$(LEX) $^
%.tab.c %.tab.h : $(YACC_FILES)
	$(YACC) -d $^
clean:
	$(RM) *.o server unit_test lex.yy.c *.tab.h *.tab.c
