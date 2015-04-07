#ifndef _TABLE_H
#define _TABLE_H

#include "uthash.h"
#include "list.h"
#include "card.h"
#include "player.h"

#define TABLE_MAX_PLAYERS 10
#define MIN_PLAYERS 3
#define MAX_TABLES 1024

#define ACTION_BET      0x01
#define ACTION_RAISE    0x02
#define ACTION_CALL     0x04
#define ACTION_CHCEK    0x08
#define ACTION_FOLD     0x10
#define ACTION_ALL_IN   0x20

typedef struct action_s {
    unsigned int action;
    unsigned int value;
} action_t;

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
    int turn;
    unsigned int action_mask;
    int minimum_bet;
    int mininum_raise;
    int raise_count;

    int pot;
    int bet;
    int small_blind;
    int big_blind;
    struct event_base *base;
    struct event *ev_timeout;
    UT_hash_handle hh;
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
int player_fold(player_t *player);
int player_check(player_t *player);
int player_bet(player_t *player, int bet);
int handle_table(table_t *table);
int handle_action(player_t *player, action_t action);
void table_reset(table_t *table);
void table_pre_flop(table_t *table);
void table_flop(table_t *table);
void table_turn(table_t *table);
void table_river(table_t *table);
void table_showdown(table_t *table);
int table_check_winner(table_t *table);
void table_init_timeout(table_t *table);
void table_reset_timeout(table_t *table);
void table_clear_timeout(table_t *table);
int table_to_json(table_t *table, char *buffer, int size);

table_t *table_create();
void tatle_destroy(table_t *table);
extern table_t *g_tables;
extern int g_num_tables;

void broadcast(table_t *table, const char *fmt, ...);
void send_msg(player_t *player, const char *fmt, ...);
void timeoutcb(evutil_socket_t fd, short events, void *arg);
extern char g_table_report_buffer[4096];
#ifdef _USE_JSON
#define report(table) do {\
    table_to_json((table), g_table_report_buffer, sizeof(g_table_report_buffer));\
    broadcast((table), "[\"update\",%s]\n", g_table_report_buffer);\
} while (0)
#else
#define report(table) do {\
    broadcast((table), "turn %s pot %d", current_player(table)->name, table->pot);\
} while (0)
#endif
#endif
