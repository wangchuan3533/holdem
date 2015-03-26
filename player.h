#ifndef _PLAYER_H
#define _PLAYER_H
#define MAX_NAME 64
#include "card.h"
#include "hand.h"

#define MAX_PLAYERS 1024
struct bufferevent;
struct table_s;

typedef enum player_state_e {
    PLAYER_STATE_EMPTY,
    PLAYER_STATE_LOGIN,
    PLAYER_STATE_NAME,
    PLAYER_STATE_WAITING,
    PLAYER_STATE_GAME,
    PLAYER_STATE_FOLDED,
    PLAYER_STATE_LOGOUT,
} player_state_t;

typedef struct player_s {
    struct bufferevent *bev;
    char name[MAX_NAME];
    int pot;
    int bid;
    struct table_s *table;
    card_t hand_cards[7];
    hand_rank_t rank;
    player_state_t state;
} player_t;

player_t *available_player();
int player_to_json(player_t *player, char *buffer, int size);
#endif
