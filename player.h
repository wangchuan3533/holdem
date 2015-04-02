#ifndef _PLAYER_H
#define _PLAYER_H
#define MAX_NAME 64
#include "uthash.h"
#include "card.h"
#include "hand.h"

#define MAX_PLAYERS 1024
struct bufferevent;
struct table_s;

#define _PLAYER_STATE_LOGIN     0x01
#define _PLAYER_STATE_TABLE     0x02
#define _PLAYER_STATE_GAME      0x04
#define _PLAYER_STATE_FOLDED    0x08

typedef unsigned int player_state_t;

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

#ifdef TEXAS_ASSERT
#define ASSERT_LOGIN(player)         assert((player)->state & _PLAYER_STATE_LOGIN)
#define ASSERT_NOT_LOGIN(player)     assert(!((player)->state & _PLAYER_STATE_LOGIN))
#define ASSERT_TABLE(player)         assert((player)->state & _PLAYER_STATE_TABLE)
#define ASSERT_NOT_TABLE(player)     assert(!((player)->state & _PLAYER_STATE_TABLE))
#define ASSERT_GAME(player)          assert((player)->state & _PLAYER_STATE_GAME)
#define ASSERT_NOT_GAME(player)      assert(!((player)->state & _PLAYER_STATE_GAME))
#define ASSERT_FOLDED(player)        assert((player)->state & _PLAYER_STATE_FOLDED)
#define ASSERT_NOT_FOLDED(player)    assert(!((player)->state & _PLAYER_STATE_FOLDED))
#else
#define ASSERT_LOGIN(player)
#define ASSERT_NOT_LOGIN(player)
#define ASSERT_TABLE(player)
#define ASSERT_NOT_TABLE(player)
#define ASSERT_GAME(player)
#define ASSERT_NOT_GAME(player)
#define ASSERT_FOLDED(player)
#define ASSERT_NOT_FOLDED(player)
#endif

#define CHECK_LOGIN (player) do {\
    if ((player)->state & _PLAYER_STATE_LOGIN == 0) {\
        send_msg((player), "you are not logged in\ntexas> ");\
        return -1;\
    }\
} while(0)

#define CHECK_NOT_LOGIN (player) do {\
    if ((player)->state & _PLAYER_STATE_LOGIN) {\
        send_msg((player), "you are already logged in as %s\ntexas> ", (player)->name);\
        return -1;\
    }\
} while(0)

#define CHECK_TABLE (player) do {\
    if ((player)->state & _PLAYER_STATE_TABLE == 0) {\
        send_msg((player), "you are not in table\ntexas> ");\
        return -1;\
    }\
} while(0)

#define CHECK_NOT_TABLE (player) do {\
    if ((player)->state & _PLAYER_STATE_TABLE) {\
        send_msg((player), "you are already in table\ntexas> ");\
        return -1;\
    }\
} while(0)

#define CHECK_GAME (player) do {\
    if ((player)->state & _PLAYER_STATE_GAME == 0) {\
        send_msg((player), "you are not in game\ntexas> ");\
        return -1;\
    }\
} while(0)

#define CHECK_NOT_GAME (player) do {\
    if ((player)->state & _PLAYER_STATE_GAME) {\
        send_msg((player), "you are already in game\ntexas> ");\
        return -1;\
    }\
} while(0)

#define CHECK_FOLDED (player) do {\
    if ((player)->state & _PLAYER_STATE_FOLDED == 0) {\
        send_msg((player), "you are not folded\ntexas> ");\
        return -1;\
    }\
} while(0)

#define CHECK_NOT_FOLDED (player) do {\
    if ((player)->state & _PLAYER_STATE_FOLDED) {\
        send_msg((player), "you are already folded\ntexas> ");\
        return -1;\
    }\
} while(0)

extern player_t *g_current_player;
extern player_t *g_players;
player_t *player_create();
void player_destroy(player_t *player);
extern int g_num_player;
void send_msg(player_t *player, const char *fmt, ...);
int player_to_json(player_t *player, char *buffer, int size);
#endif
