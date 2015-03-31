#ifndef _PLAYER_H
#define _PLAYER_H
#define MAX_NAME 64
#include "uthash.h"
#include "card.h"
#include "hand.h"

#define MAX_PLAYERS 1024
struct bufferevent;
struct table_s;

typedef enum player_state_e {
    PLAYER_STATE_NEW,
    PLAYER_STATE_LOGIN,
    PLAYER_STATE_WAITING,
    PLAYER_STATE_GAME,
    PLAYER_STATE_FOLDED,
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
    UT_hash_handle hh;
} player_t;

extern player_t *g_current_player;
extern player_t *g_players;
player_t *player_create();
void player_destroy(player_t *player);
extern int g_num_player;
int player_to_json(player_t *player, char *buffer, int size);
#endif
