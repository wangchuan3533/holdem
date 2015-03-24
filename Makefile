CC=gcc
RM=rm -rf
C_FLAGS=-g -Wall
C_FLAGS+=-Werror
C_FLAGS+=-I$(HOME)/.jumbo/include
LD_FLAGS=-levent
LD_FLAGS+=-L$(HOME)/.jumbo/lib
HEADERS=$(wildcard *.h)
SOURCES=$(wildcard *.c)
OBJS=$(SOURCES:.c=.o)

.PHONY: clean
all: main

test: C_FLAGS+=-DUNIT_TEST
test: test

main : $(OBJS)
	$(CC) $^ $(LD_FLAGS) -o $@
test : $(OBJS)
	$(CC) $^ $(LD_FLAGS) -o $@
%.o: %.c $(HEADERS)
	$(CC) $(C_FLAGS) -c $< -o $@
clean:
	$(RM) *.o main test
