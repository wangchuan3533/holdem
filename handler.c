#include "texas_holdem.h"
#include "handler.h"
#include "card.h"
#include "player.h"
#include "hand.h"
#include "table.h"

int login(const char *name)
{
    
    player_t *tmp, *player = g_current_player;

    CHECK_NOT_LOGIN(player);
    ASSERT_NOT_TABLE(player);
    ASSERT_NOT_GAME(player);

    HASH_FIND(hh, g_players, name, strlen(name), tmp);
    if (tmp) {
        send_msg(player, "name %s already exist\ntexas> ", name);
        return -1;
    }
    strncpy(player->name, name, sizeof(player->name));
    HASH_ADD(hh, g_players, name, strlen(player->name), player);
    player->state |= PLAYER_STATE_LOGIN;
    send_msg(player, "welcome to texas holdem, %s\ntexas> ", player->name);
    return 0;
}

int logout()
{
    player_t *tmp, *player = g_current_player;

    CHECK_LOGIN(player);

    if (player->state & PLAYER_STATE_TABLE) {
        assert(quit_table() == 0);
    }
    HASH_FIND(hh, g_players, player->name, strlen(player->name), tmp);
    assert(tmp == player);
    HASH_DELETE(hh, g_players, player);
    send_msg(player, "bye %s\ntexas> ", player->name);
    player->state &= ~PLAYER_STATE_LOGIN;
    return 0;
}

int create_table(const char *name)
{
    table_t *table, *tmp;
    player_t *player = g_current_player;

    CHECK_LOGIN(player);
    CHECK_NOT_TABLE(player);

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
    table->base = bufferevent_get_base(player->bev);
    table_init_timeout(table);
    strncpy(table->name, name, sizeof(table->name));
    HASH_ADD(hh, g_tables, name, strlen(table->name), table);
    if (player_join(table, player) < 0) {
        send_msg(player, "join table %s failed\ntexas> ", table->name);
        return -1;
    }

    table_reset(table);
    //send_msg(player, "table %s created\ntexas> ", table->name);
    return 0;
}

int join_table(const char *name)
{
    table_t *table;
    player_t *player = g_current_player;

    CHECK_LOGIN(player);
    CHECK_NOT_TABLE(player);

    HASH_FIND(hh, g_tables, name, strlen(name), table);
    if (!table) {
        send_msg(player, "table %s dose not exist\ntexas> ", name);
        return -1;
    }

    if (player_join(table, player) < 0) {
        send_msg(player, "join table %s failed\ntexas> ", table->name);
        return -1;
    }

    if (table->num_players >= MIN_PLAYERS && table->state == TABLE_STATE_WAITING) {
        handle_table(table);
    }
    //send_msg(player, "join table %s success\ntexas> ", table->name);
    return 0;
}

int quit_table()
{
    player_t *player = g_current_player;
    table_t *table = player->table;

    CHECK_LOGIN(player);
    CHECK_TABLE(player);

    if (player->state & PLAYER_STATE_GAME) {
        fold();
    }
    assert(player_quit(player) == 0);
    send_msg(player, "quit table %s success\ntexas> ", table->name);
    return 0;
}

int show_tables()
{
    player_t *player = g_current_player;
    table_t *table, *tmp;

    CHECK_LOGIN(player);
    HASH_ITER(hh, g_tables, table, tmp) {
        send_msg(player, "%s ", table->name);
    }
    send_msg(player, "\ntexas> ");
    return 0;
}

int show_players()
{
    player_t *tmp1, *tmp2, *player = g_current_player;

    HASH_ITER(hh, g_players, tmp1, tmp2) {
        send_msg(player, "%s ", tmp1->name);
    }
    send_msg(player, "\ntexas> ");
    return 0;
}

int show_players_in_table(const char *name)
{
    table_t *table;
    player_t *player = g_current_player;
    int i;

    CHECK_LOGIN(player);
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

int pwd()
{
    player_t *player = g_current_player;

    if (player->state & PLAYER_STATE_TABLE) {
        send_msg(player, "%s\ntexas> ", player->table->name);
        return 0;
    }
    send_msg(player, "root\ntexas> ");
    return 0;
}

int raise(int bet)
{
    player_t *player = g_current_player;

    CHECK_LOGIN(player);
    CHECK_TABLE(player);
    CHECK_GAME(player);
    CHECK_IN_TURN(player);

    if (player_bet(player, bet) != 0) {
        send_msg(player, "raise %d failed\ntexas> ", bet);
        return -1;
    }
    broadcast(player->table, "%s raise %d\ntexas> ", player->name, bet);
    player->table->turn = next_player(player->table, player->table->turn);
    table_reset_timeout(player->table);
    report(player->table);
    return 0;
}

int call()
{
    player_t *player = g_current_player;
    int bet;

    CHECK_LOGIN(player);
    CHECK_TABLE(player);
    CHECK_GAME(player);
    CHECK_IN_TURN(player);

    bet = player->table->bet - player->bet;
    if (player_bet(player, bet) != 0) {
        send_msg(player, "call %d failed\ntexas> ", bet);
        return -1;
    }
    if (player->table->players[next_player(player->table, player->table->turn)]->bet == player->table->bet) {
        handle_table(player->table);
        return 0;
    }
    broadcast(player->table, "%s call %d\ntexas> ", player->name, bet);
    player->table->turn = next_player(player->table, player->table->turn);
    table_reset_timeout(player->table);
    report(player->table);
    return 0;
}

int fold()
{
    player_t *player = g_current_player;

    CHECK_LOGIN(player);
    CHECK_TABLE(player);
    CHECK_GAME(player);
    //CHECK_IN_TURN(player);

    assert(player_fold(player) == 0);
    broadcast(player->table, "%s fold\ntexas> ", player->name);
    if (player->table->num_playing == 1) {
        assert(table_check_winner(player->table) == 0);
        handle_table(player->table);
        return 0;
    }

    if (current_player(player->table) == player) {
        player->table->turn = next_player(player->table, player->table->turn);
        table_reset_timeout(player->table);
    }
    report(player->table);
    return 0;
}

int check()
{
    player_t *player = g_current_player;

    CHECK_LOGIN(player);
    CHECK_TABLE(player);
    CHECK_GAME(player);
    CHECK_IN_TURN(player);

    if (player_check(player) < 0) {
        send_msg(player, "check failed\ntexas> ");
        return -1;
    }
    if (player->table->turn == player->table->dealer) {
        handle_table(player->table);
        return 0;
    }

    broadcast(player->table, "%s check\ntexas> ", player->name);
    player->table->turn = next_player(player->table, player->table->turn);
    table_reset_timeout(player->table);
    report(player->table);
    return 0;
}

int yyerror(char *s)
{
    player_t *player = g_current_player;
    send_msg(player, "%s\ntexas> ", s);
    return 0;
}

int print_help()
{
    player_t *player = g_current_player;
    const char *usage =
        "login <user_name>\n"
        "logout\n"
        "mk <table_name>\n"
        "join <table_name>\n"
        "quit\n"
        "raise <num>\n"
        "call\n"
        "check\n"
        "fold\n"
        "help\n"
    ;

    evbuffer_add_printf(bufferevent_get_output(player->bev), "%s\ntexas> ", usage);
    return 0;
}

int reply(const char *fmt, ...)
{
    va_list ap;
    player_t *player = g_current_player;

    va_start(ap, fmt);
    evbuffer_add_vprintf(bufferevent_get_output(player->bev), fmt, ap);
    va_end(ap);
    return 0;
}

int chat(const char *str)
{
    player_t *player = g_current_player;

    CHECK_LOGIN(player);
    CHECK_TABLE(player);

    broadcast(player->table, "[CHAT] %s: %s\ntexas> ", player->name, str);

    return 0;
}
