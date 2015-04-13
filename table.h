#ifndef _TABLE_H
#define _TABLE_H

#include "uthash.h"
#include "list.h"
#include "card.h"
#include "hand.h"
#include "user.h"

#define TABLE_TMP_BUFFER_SIZE 1024
#define TABLE_MAX_PLAYERS 10
#define MIN_PLAYERS 3
#define MAX_TABLES 1024

typedef enum action_e {
    ACTION_BET     = 0x01,
    ACTION_RAISE   = 0x02,
    ACTION_CALL    = 0x04,
    ACTION_CHCEK   = 0x08,
    ACTION_FOLD    = 0x10,
    ACTION_ALL_IN  = 0x20,
} action_t;
int action_to_string(action_t mask, char *buffer, int size);

typedef enum player_state_e
{
    PLAYER_STATE_EMPTY = 0,
    PLAYER_STATE_WAITING,
    PLAYER_STATE_ACTIVE,
    PLAYER_STATE_ALL_IN,
    PLAYER_STATE_FOLDED,
} player_state_t;

typedef struct player_s {
    int bet;
    int chips;
    int talked;
    int pot_offset;
    player_state_t state;
    card_t hand_cards[7];
    hand_rank_t rank;
    user_t *user;
} player_t;

typedef enum table_state_e {
    TABLE_STATE_WAITING = 0,
    TABLE_STATE_PREFLOP,
    TABLE_STATE_FLOP,
    TABLE_STATE_TURN,
    TABLE_STATE_RIVER,
} table_state_t;

typedef struct table_s {
    char name[MAX_NAME];
    table_state_t state;
    deck_t deck;
    card_t community_cards[5];

    player_t *players[TABLE_MAX_PLAYERS];

    int num_players;
    int num_all_in;
    int num_folded;
    int num_active;
    int num_available;

    int dealer;
    int small_blind;
    int big_blind;
    int turn;

    unsigned int action_mask;
    int minimum_bet;
    int minimum_raise;

    int bet;
    int pot;
    int side_pots[TABLE_MAX_PLAYERS];
    int pot_count;

    struct event_base *base;
    struct event *ev_timeout;
    UT_hash_handle hh;
    char buffer[TABLE_TMP_BUFFER_SIZE];
} table_t;

#define current_player(t) ((t)->players[(t)->turn])
#define CHECK_IN_TURN(user) do {\
    if (((user)->index) != (user)->table->turn) {\
        send_msg((user), "you are not in turn");\
        return -1;\
    }\
} while(0)

int player_join(table_t *table, user_t *user);
int player_quit(user_t *user);

int next_player(table_t *table, int index);
int player_bet(table_t *table, int index, int bet);
int player_raise(table_t *table, int index, int raise);
int player_call(table_t *table, int index);
int player_fold(table_t *table, int index);
int player_check(table_t *table, int index);
int player_all_in(table_t *table, int index);

table_t *table_create();
void tatle_destroy(table_t *table);

void table_prepare(table_t *table);
void table_pre_flop(table_t *table);
void table_flop(table_t *table);
void table_turn(table_t *table);
void table_river(table_t *table);
void table_start(table_t *table);
void table_finish(table_t *table);

void table_init_timeout(table_t *table);
void table_reset_timeout(table_t *table, int left);
void table_clear_timeout(table_t *table);

int handle_action(table_t *table, int index, action_t action, int value);
int table_process(table_t *table);
int table_switch(table_t *table);

void broadcast(table_t *table, const char *fmt, ...);
void timeoutcb(evutil_socket_t fd, short events, void *arg);
#define report(table) do {\
    action_to_string((table)->action_mask, (table)->buffer, sizeof((table)->buffer));\
    broadcast((table), "turn %s bet %d pot %d mask %s chips %d", current_player(table)->user->name, table->bet, table->pot, (table)->buffer, current_player(table)->chips);\
} while (0)

extern table_t *g_tables;
extern int g_num_tables;
#endif
