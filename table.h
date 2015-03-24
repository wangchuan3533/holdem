#ifndef _TABLE_H
#define _TABLE_H

#include "list.h"
#include "card.h"
#include "player.h"

#define MAX_PLAYERS 128
#define MIN_PLAYERS 3

typedef enum table_state_e {
    TABLE_STATE_WAITING,
    TABLE_STATE_PREFLOP,
    TABLE_STATE_FLOP,
    TABLE_STATE_TURN,
    TABLE_STATE_RIVER,
    TABLE_STATE_SHOWDOWN,
} table_state_t;

typedef struct table_s {
    table_state_t state;

    deck_t deck;
    card_t community_cards[5];

    list_t waiting_players;
    list_t gaming_players;
    list_t folded_playsers;

    list_node_t *turn;

    int pot;
    int bid;
    int small_blind;
    int big_blind;
    int minimum_bet;
} table_t;

#define current_player(t) ((player_t *)((t)->turn->data))
#define next_player(t) ((player_t *)(((t)->turn->next) ? (t)->turn->next->data : (t)->gaming_players.head->data))
#define prev_player(t) ((player_t *)(((t)->turn->prev) ? (t)->turn->prev->data : (t)->gaming_players.tail->data))
#define move_next(t) do {\
    if ((t)->turn == (t)->gaming_players.tail) {\
        (t)->turn = (t)->gaming_players.head;\
    } else {\
        (t)->turn = (t)->turn->next;\
    }\
} while (0)

void table_init(table_t *table);

void table_pre_flop(table_t *table);
void table_flop(table_t *table);
void table_turn(table_t *table);
void table_river(table_t *table);
void table_showdown(table_t *table);

void broadcast(table_t *table, const char *fmt, ...);
void send_msg(player_t *player, const char *fmt, ...);

#define report(table, player) do {\
    broadcast((table), "[PLAYER]%s [POT]%d [BID]%d [STATE]%d\n", (player)->name, (player)->pot, (player)->bid, (player)->state);\
    broadcast((table), "[TABLE] [POT]%d [BID]%d [STATE]%s\n", (table)->pot, (table)->bid, (player)->state);\
    broadcast((table), "[TABLE] NEXT IS %s\n", ((player_t *)((table)->turn->data))->name);\
} while (0)

#endif
