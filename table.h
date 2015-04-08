#ifndef _TABLE_H
#define _TABLE_H

#include "uthash.h"
#include "list.h"
#include "card.h"
#include "player.h"

#define TABLE_TMP_BUFFER_SIZE 1024
#define TABLE_MAX_PLAYERS 10
#define MIN_PLAYERS 3
#define MAX_TABLES 1024

#define ACTION_BET      0x01
#define ACTION_RAISE    0x02
#define ACTION_CALL     0x04
#define ACTION_CHCEK    0x08
#define ACTION_FOLD     0x10
#define ACTION_ALL_IN   0x20

typedef unsigned int action_t;

typedef enum table_state_e {
    TABLE_STATE_WAITING,
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
    int num_playing;
    int dealer;
    int small_blind;
    int big_blind;
    int turn;
    unsigned int action_mask;
    int minimum_bet;
    int minimum_raise;
    int raise_count;

    int pot;
    int bet;
    struct event_base *base;
    struct event *ev_timeout;
    UT_hash_handle hh;
    char buffer[TABLE_TMP_BUFFER_SIZE];
} table_t;

#define current_player(t) ((t)->players[(t)->turn])
#define CHECK_IN_TURN(player) do {\
    if ((player) != current_player((player)->table)) {\
        send_msg((player), "you are not in turn");\
        return -1;\
    }\
} while(0)

int player_join(table_t *table, player_t *player);
int player_quit(player_t *player);
int next_player(table_t *table, int index);
int player_bet(player_t *player, int bet);
int player_raise(player_t *player, int raise);
int player_call(player_t *player);
int player_fold(player_t *player);
int player_check(player_t *player);
int player_all_in(player_t *player);

table_t *table_create();
void tatle_destroy(table_t *table);

void table_reset(table_t *table);
void table_pre_flop(table_t *table);
void table_flop(table_t *table);
void table_turn(table_t *table);
void table_river(table_t *table);
void table_showdown(table_t *table);
void table_init_timeout(table_t *table);
void table_reset_timeout(table_t *table);
void table_clear_timeout(table_t *table);
int table_check_winner(table_t *table);
int table_to_json(table_t *table, char *buffer, int size);

extern table_t *g_tables;
extern int g_num_tables;

int handle_action(player_t *player, action_t action, int value);
int handle_table(table_t *table);

void broadcast(table_t *table, const char *fmt, ...);
void timeoutcb(evutil_socket_t fd, short events, void *arg);
int action_to_string(action_t mask, char *buffer, int size);
#ifdef _USE_JSON
#define report(table) do {\
    table_to_json((table), (table)->buffer, sizeof((table)->buffer));\
    broadcast((table), "[\"update\",%s]\n", (table)->buffer);\
} while (0)
#else
#define report(table) do {\
    action_to_string((table)->action_mask, (table)->buffer, sizeof((table)->buffer));\
    broadcast((table), "turn %s bet %d pot %d mask %s", current_player(table)->name, table->bet, table->pot, (table)->buffer);\
} while (0)
#endif
#endif
