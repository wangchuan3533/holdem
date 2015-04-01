#include <assert.h>
#include <stdarg.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include "handler.h"
#include "card.h"
#include "player.h"
#include "hand.h"
#include "table.h"

int login(const char *name)
{
    
    player_t *tmp, *player = g_current_player;

    if (player->state != PLAYER_STATE_NEW) {
        send_msg(player, "you already logged in\ntexas> ");
        return -1;
    }
    HASH_FIND(hh, g_players, name, strlen(name), tmp);
    if (tmp) {
        send_msg(player, "name %s already exist\ntexas> ", name);
        return -1;
    }
    strncpy(player->name, name, sizeof(player->name));
    HASH_ADD(hh, g_players, name, strlen(player->name), player);
    player->state = PLAYER_STATE_LOGIN;
    send_msg(player, "welcome to texas holdem, %s\ntexas> ", player->name);
    return 0;
}

int logout()
{
    
    player_t *tmp, *player = g_current_player;

    if (player->state == PLAYER_STATE_NEW) {
        send_msg(player, "you are not logged in\ntexas> ");
        return -1;
    }
    HASH_FIND(hh, g_players, player->name, strlen(player->name), tmp);
    assert(tmp == player);
    HASH_DELETE(hh, g_players, player);
    if (player->table) {
        del_player(player->table, player);
    }
    send_msg(player, "bye %s\ntexas> ", player->name);
    player->state = PLAYER_STATE_NEW;
    return 0;
}

int create_table(const char *name)
{
    table_t *table, *tmp;
    player_t *player = g_current_player;

    if (player->state == PLAYER_STATE_NEW) {
        send_msg(player, "you are not logged in\ntexas> ");
        return -1;
    }
    if (player->state != PLAYER_STATE_LOGIN) {
        send_msg(player, "you are in table %s\ntexas> ", player->table->name);
        return -1;
    }
    HASH_FIND(hh, g_tables, name, strlen(name), tmp);
    if (tmp) {
        send_msg(player, "table %s already exist\ntexas> ", name);
        return -1;
    }

    table = table_create();
    if (table == NULL) {
        send_msg(player, "table %s created failed\ntexas> ", name);
        return -1;
    }
    strncpy(table->name, name, sizeof(table->name));
    HASH_ADD(hh, g_tables, name, strlen(table->name), table);
    if (add_player(table, player) < 0) {
        send_msg(player, "join table %s failed\ntexas> ", table->name);
        return -1;
    }
    //send_msg(player, "table %s created\ntexas> ", table->name);
    return 0;
}

int join_table(const char *name)
{
    table_t *table;
    player_t *player = g_current_player;

    if (player->state == PLAYER_STATE_NEW) {
        send_msg(player, "you are not logged in\ntexas> ");
        return -1;
    }
    if (player->state != PLAYER_STATE_LOGIN) {
        send_msg(player, "you are in table %s\ntexas> ", player->table->name);
        return -1;
    }
    HASH_FIND(hh, g_tables, name, strlen(name), table);
    if (!table) {
        send_msg(player, "table %s dose not exist\ntexas> ", name);
        return -1;
    }

    if (add_player(table, player) < 0) {
        send_msg(player, "join table %s failed\ntexas> ", table->name);
        return -1;
    }
    //send_msg(player, "join table %s success\ntexas> ", table->name);
    return 0;
}

int quit_table()
{
    player_t *player = g_current_player;
    table_t *table = player->table;

    if (player->state == PLAYER_STATE_NEW) {
        send_msg(player, "you are not logged in\ntexas> ");
        return -1;
    }
    if (player->state == PLAYER_STATE_LOGIN) {
        send_msg(player, "you are not in a table\ntexas> ");
        return -1;
    }

    if (del_player(table, player) < 0) {
        send_msg(player, "quit table %s failed\ntexas> ", table->name);
        return -1;
    }
    send_msg(player, "quit table %s success\ntexas> ", table->name);
    return 0;
}

int ls(const char *name)
{
    table_t *table, *tmp;
    player_t *player = g_current_player;
    int i;

    if (player->state == PLAYER_STATE_NEW) {
        send_msg(player, "you are not logged in\ntexas> ");
        return -1;
    }
    if (player->state == PLAYER_STATE_LOGIN) {
        if (name == NULL) {
            HASH_ITER(hh, g_tables, table, tmp) {
                send_msg(player, "%s ", table->name);
            }
            send_msg(player, "\ntexas> ");
            return 0;
        }

        HASH_FIND(hh, g_tables, name, strlen(name), table);
        if (!table) {
            send_msg(player, "table %s dose not exist\ntexas> ", name);
            return -1;
        }
        for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
            if (table->players[i]) {
                send_msg(player, "%s ", table->players[i]->name);
            }
        }
        send_msg(player, "\ntexas> ");
        return 0;
    }

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (player->table->players[i]) {
            send_msg(player, "%s ", player->table->players[i]->name);
        }
    }
    send_msg(player, "\ntexas> ");
    return 0;
}

int pwd()
{
    player_t *player = g_current_player;
    if (player->state == PLAYER_STATE_NEW) {
        send_msg(player, "you are not logged in\ntexas> ");
        return -1;
    }
    if (player->state == PLAYER_STATE_LOGIN) {
        send_msg(player, "root\ntexas> ");
        return 0;
    }
    send_msg(player, "%s\ntexas> ", player->table->name);
    return 0;
}

int raise(int bid)
{
    player_t *player = g_current_player;

    if (player->state == PLAYER_STATE_NEW) {
        send_msg(player, "you are not logged in\ntexas> ");
        return -1;
    }
    if (player->state == PLAYER_STATE_LOGIN) {
        send_msg(player, "you are not in table\ntexas> ");
        return -1;
    }

    if (player != current_player(player->table) || player->state != PLAYER_STATE_GAME) {
        send_msg(player, "you are not in turn\ntexas> ");
        return -1;
    }

    if (player_bet(player, bid) != 0) {
        send_msg(player, "raise %d failed\ntexas> ", bid);
        return -1;
    }
    broadcast(player->table, "%s raise %d\ntexas> ", player->name, bid);
    player->table->turn = next_player(player->table, player->table->turn);
    report(player->table);
    return 0;
}

int call()
{
    player_t *player = g_current_player;
    int bid;

    if (player->state == PLAYER_STATE_NEW) {
        send_msg(player, "you are not logged in\ntexas> ");
        return -1;
    }
    if (player->state == PLAYER_STATE_LOGIN) {
        send_msg(player, "you are not in table\ntexas> ");
        return -1;
    }

    if (player != current_player(player->table) || player->state != PLAYER_STATE_GAME) {
        send_msg(player, "you are not in turn\ntexas> ");
        return -1;
    }

    bid = player->table->bid - player->bid;
    if (player_bet(player, bid) != 0) {
        send_msg(player, "call %d failed\ntexas> ", bid);
        return -1;
    }
    if (player->table->players[next_player(player->table, player->table->turn)]->bid == player->table->bid) {
        handle_table(player->table);
        return 0;
    }
    broadcast(player->table, "%s call %d\ntexas> ", player->name, bid);
    player->table->turn = next_player(player->table, player->table->turn);
    report(player->table);
    return 0;
}

int fold()
{
    player_t *player = g_current_player;

    if (player->state == PLAYER_STATE_NEW) {
        send_msg(player, "you are not logged in\ntexas> ");
        return -1;
    }
    if (player->state == PLAYER_STATE_LOGIN) {
        send_msg(player, "you are not in table\ntexas> ");
        return -1;
    }

    if (player != current_player(player->table) || player->state != PLAYER_STATE_GAME) {
        send_msg(player, "you are not in turn\ntexas> ");
        return -1;
    }

    if (player_fold(player) != 0) {
        send_msg(player, "fold failed\ntexas> ");
        return -1;
    }

    if (table_check_winner(player->table) < 0) {
        broadcast(player->table, "%s fold\ntexas> ", player->name);
        player->table->turn = next_player(player->table, player->table->turn);
        report(player->table);
    }
    return 0;
}

int check()
{
    player_t *player = g_current_player;

    if (player->state == PLAYER_STATE_NEW) {
        send_msg(player, "you are not logged in\ntexas> ");
        return -1;
    }
    if (player->state == PLAYER_STATE_LOGIN) {
        send_msg(player, "you are not in table\ntexas> ");
        return -1;
    }

    if (player != current_player(player->table) || player->state != PLAYER_STATE_GAME) {
        send_msg(player, "you are not in turn\ntexas> ");
        return -1;
    }

    if (player_check(player) != 0) {
        send_msg(player, "check failed\ntexas> ");
        return -1;
    }
    if (player->table->turn == player->table->dealer) {
        handle_table(player->table);
        return 0;
    }

    broadcast(player->table, "%s check\ntexas> ", player->name);
    player->table->turn = next_player(player->table, player->table->turn);
    report(player->table);
    return 0;
}

int yyerror(char *s)
{
    player_t *player = g_current_player;
    send_msg(player, "%s\ntexas> ", s);
    return 0;
}

int reply(const char *fmt, ...)
{
    va_list ap;
    player_t *player = g_current_player;

    if (player->state == PLAYER_STATE_NEW) {
        send_msg(player, "you are not logged in\ntexas> ");
        return -1;
    }

    va_start(ap, fmt);
    evbuffer_add_vprintf(bufferevent_get_output(player->bev), fmt, ap);
    va_end(ap);
    return 0;
}


