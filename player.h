#ifndef _PLAYER_H
#define _PLAYER_H
#define MAX_NAME 64
#include "uthash.h"
#include "card.h"
#include "hand.h"

#define MAX_PLAYERS 1024
struct bufferevent;
struct table_s;

#define PLAYER_STATE_LOGIN     0x01
#define PLAYER_STATE_TABLE     0x02
#define PLAYER_STATE_GAME      0x04

typedef unsigned int player_state_t;

typedef struct player_s {
    struct bufferevent *bev;
    char name[MAX_NAME];
    char prompt[MAX_NAME];
    int pot;
    int bet;
    int chips;
    int all_in;
    struct table_s *table;
    card_t hand_cards[7];
    hand_rank_t rank;
    player_state_t state;
    UT_hash_handle hh;
} player_t;

#ifdef TEXAS_ASSERT
#define ASSERT_LOGIN(player)         assert((player)->state & PLAYER_STATE_LOGIN)
#define ASSERT_NOT_LOGIN(player)     assert(!((player)->state & PLAYER_STATE_LOGIN))
#define ASSERT_TABLE(player)         assert((player)->state & PLAYER_STATE_TABLE)
#define ASSERT_NOT_TABLE(player)     assert(!((player)->state & PLAYER_STATE_TABLE))
#define ASSERT_GAME(player)          assert((player)->state & PLAYER_STATE_GAME)
#define ASSERT_NOT_GAME(player)      assert(!((player)->state & PLAYER_STATE_GAME))
#else
#define ASSERT_LOGIN(player)
#define ASSERT_NOT_LOGIN(player)
#define ASSERT_TABLE(player)
#define ASSERT_NOT_TABLE(player)
#define ASSERT_GAME(player)
#define ASSERT_NOT_GAME(player)
#endif

#define CHECK_LOGIN(player) do {\
    if (!((player)->state & PLAYER_STATE_LOGIN)) {\
        send_msg((player), "you are not logged in");\
        return -1;\
    }\
} while(0)

#define CHECK_NOT_LOGIN(player) do {\
    if ((player)->state & PLAYER_STATE_LOGIN) {\
        send_msg((player), "you are already logged in as %s", (player)->name);\
        return -1;\
    }\
} while(0)

#define CHECK_TABLE(player) do {\
    if (!((player)->state & PLAYER_STATE_TABLE)) {\
        send_msg((player), "you are not in table");\
        return -1;\
    }\
} while(0)

#define CHECK_NOT_TABLE(player) do {\
    if ((player)->state & PLAYER_STATE_TABLE) {\
        send_msg((player), "you are already in table");\
        return -1;\
    }\
} while(0)

#define CHECK_GAME(player) do {\
    if (!((player)->state & PLAYER_STATE_GAME)) {\
        send_msg((player), "you are not in game");\
        return -1;\
    }\
} while(0)

#define CHECK_NOT_GAME(player) do {\
    if ((player)->state & PLAYER_STATE_GAME) {\
        send_msg((player), "you are already in game");\
        return -1;\
    }\
} while(0)

extern player_t *g_current_player;
extern player_t *g_players;
extern int g_num_player;
player_t *player_create();
void player_destroy(player_t *player);
void send_msg(player_t *player, const char *fmt, ...);
void send_msgv(player_t *player, const char *fmt, va_list ap);
int player_to_json(player_t *player, char *buffer, int size);
#endif
