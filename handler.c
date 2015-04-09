#include "texas_holdem.h"
#include "handler.h"
#include "card.h"
#include "user.h"
#include "hand.h"
#include "table.h"
#include "sha1.h"

int reg(const char *name, const char *password)
{
    user_t *user = g_current_user;

    CHECK_NOT_LOGIN(user);

    if (user_load(name, user) == 0) {
        send_msg(user, "name %s already exist", name);
        return -1;
    }

    strncpy(user->name, name, sizeof(user->name));
    snprintf(user->prompt, sizeof(user->prompt), "\n>");
    sha1(user->password, password, strlen(password) << 3);
    user->money = 100000;

    if (user_save(user) < 0) {
        send_msg(user, "save user to db failed %s", name);
        return -1;
    }

    HASH_ADD(hh, g_users, name, strlen(user->name), user);
    user->state |= USER_STATE_LOGIN;
    send_msg(user, "welcome to texas holdem, %s", user->name);
    return 0;
}

int login(const char *name, const char *password)
{
    
    user_t *tmp, *user = g_current_user;
    char sha1_buf[20];

    CHECK_NOT_LOGIN(user);
    ASSERT_NOT_TABLE(user);

    if (user_load(name, user) < 0) {
        send_msg(user, "user %s dose not exist", name);
        return -1;
    }
    sha1(sha1_buf, password, strlen(password) << 3);
    if (bcmp(sha1_buf, user->password, 20) != 0) {
        send_msg(user, "wrong password");
        return -1;
    }
    HASH_FIND(hh, g_users, name, strlen(name), tmp);
    if (tmp) {
        send_msg(user, "name %s already login", name);
        return -1;
    }
    HASH_ADD(hh, g_users, name, strlen(user->name), user);
    user->state |= USER_STATE_LOGIN;
    send_msg(user, "welcome to texas holdem, %s", user->name);
    return 0;
}

int logout()
{
    user_t *tmp, *user = g_current_user;

    CHECK_LOGIN(user);

    if (user->state & USER_STATE_TABLE) {
        assert(quit_table() == 0);
    }
    HASH_FIND(hh, g_users, user->name, strlen(user->name), tmp);
    assert(tmp == user);
    HASH_DELETE(hh, g_users, user);

    if (user_save(user) < 0) {
        send_msg(user, "save user to db failed %s", user->name);
        return -1;
    }

    send_msg(user, "bye %s", user->name);
    user->state &= ~USER_STATE_LOGIN;
    return 0;
}

int create_table(const char *name)
{
    table_t *table, *tmp;
    user_t *user = g_current_user;

    CHECK_LOGIN(user);
    CHECK_NOT_TABLE(user);

    HASH_FIND(hh, g_tables, name, strlen(name), tmp);
    if (tmp) {
        send_msg(user, "table %s already exist", name);
        return -1;
    }

    table = table_create();
    if (table == NULL) {
        send_msg(user, "table %s created failed", name);
        return -1;
    }
    table->base = bufferevent_get_base(user->bev);
    table_init_timeout(table);
    strncpy(table->name, name, sizeof(table->name));
    HASH_ADD(hh, g_tables, name, strlen(table->name), table);
    if (player_join(table, user) < 0) {
        send_msg(user, "join table %s failed", table->name);
        return -1;
    }

    table_reset(table);
    //send_msg(user, "table %s created", table->name);
    return 0;
}

int join_table(const char *name)
{
    table_t *table;
    user_t *user = g_current_user;

    CHECK_LOGIN(user);
    CHECK_NOT_TABLE(user);

    HASH_FIND(hh, g_tables, name, strlen(name), table);
    if (!table) {
        send_msg(user, "table %s dose not exist", name);
        return -1;
    }

    if (player_join(table, user) < 0) {
        send_msg(user, "join table %s failed", table->name);
        return -1;
    }

    //send_msg(user, "join table %s success", table->name);
    return 0;
}

int quit_table()
{
    user_t *user = g_current_user;
    table_t *table = user->table;

    CHECK_LOGIN(user);
    CHECK_TABLE(user);

    assert(player_quit(user) == 0);
    send_msg(user, "quit table %s success", table->name);
    return 0;
}

int exit_game()
{
    user_t *user = g_current_user;

    if (user->state & USER_STATE_LOGIN) {
        assert(logout() == 0);
    }
    send_msg(user, "bye");
    bufferevent_free(user->bev);
    user_destroy(user);
    return 0;
}

int show_tables()
{
    user_t *user = g_current_user;
    table_t *table, *tmp;

    CHECK_LOGIN(user);
    HASH_ITER(hh, g_tables, table, tmp) {
        send_msg_raw(user, "%s ", table->name);
    }
    send_msg(user, "");
    return 0;
}

int show_players()
{
    user_t *tmp1, *tmp2, *user = g_current_user;

    HASH_ITER(hh, g_users, tmp1, tmp2) {
        send_msg_raw(user, "%s ", tmp1->name);
    }
    send_msg(user, "");
    return 0;
}

int show_players_in_table(const char *name)
{
    table_t *table;
    user_t *user = g_current_user;
    int i;

    CHECK_LOGIN(user);
    HASH_FIND(hh, g_tables, name, strlen(name), table);
    if (!table) {
        send_msg(user, "table %s dose not exist", name);
        return -1;
    }
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->user) {
            send_msg_raw(user, "%s ", table->players[i]->user->name);
        }
    }
    send_msg(user, "");
    return 0;
}

int pwd()
{
    user_t *user = g_current_user;

    if (user->state & USER_STATE_TABLE) {
        send_msg(user, "%s", user->table->name);
        return 0;
    }
    send_msg(user, "root");
    return 0;
}

int start()
{
    CHECK_LOGIN(g_current_user);
    CHECK_TABLE(g_current_user);
    table_reset(g_current_user->table);
    handle_table(g_current_user->table);
    return 0;
}

int bet(int bet)
{
    CHECK_LOGIN(g_current_user);
    CHECK_TABLE(g_current_user);
    CHECK_IN_TURN(g_current_user);
    return handle_action(g_current_user->table, g_current_user->index, ACTION_BET, bet);
}

int raise_(int raise)
{
    CHECK_LOGIN(g_current_user);
    CHECK_TABLE(g_current_user);
    CHECK_IN_TURN(g_current_user);
    return handle_action(g_current_user->table, g_current_user->index, ACTION_RAISE, raise);
}
int call()
{
    CHECK_LOGIN(g_current_user);
    CHECK_TABLE(g_current_user);
    CHECK_IN_TURN(g_current_user);
    return handle_action(g_current_user->table, g_current_user->index, ACTION_CALL, 0);
}
int fold()
{
    CHECK_LOGIN(g_current_user);
    CHECK_TABLE(g_current_user);
    CHECK_IN_TURN(g_current_user);
    return handle_action(g_current_user->table, g_current_user->index, ACTION_FOLD, 0);
}
int check()
{
    CHECK_LOGIN(g_current_user);
    CHECK_TABLE(g_current_user);
    CHECK_IN_TURN(g_current_user);
    return handle_action(g_current_user->table, g_current_user->index, ACTION_CHCEK, 0);
}
int all_in()
{
    CHECK_LOGIN(g_current_user);
    CHECK_TABLE(g_current_user);
    CHECK_IN_TURN(g_current_user);
    return handle_action(g_current_user->table, g_current_user->index, ACTION_ALL_IN, 0);
}

int yyerror(char *s)
{
    user_t *user = g_current_user;
    send_msg(user, "%s", s);
    return 0;
}

int print_help()
{
    user_t *user = g_current_user;
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

    send_msg(user, "%s", usage);
    return 0;
}

int reply(const char *fmt, ...)
{
    va_list ap;
    user_t *user = g_current_user;

    va_start(ap, fmt);
    send_msgv(user, fmt, ap);
    va_end(ap);
    return 0;
}

int chat(const char *str)
{
    user_t *user = g_current_user;

    CHECK_LOGIN(user);
    CHECK_TABLE(user);
    broadcast(user->table, "[CHAT] %s: %s", user->name, str);

    return 0;
}

int prompt(const char *str)
{
    user_t *user = g_current_user;

    snprintf(user->prompt, sizeof(user->prompt), "\n%s", str);
    send_msg(user, "set prompt success");
    return 0;
}
