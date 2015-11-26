#ifndef _USER_H
#define _USER_H
#define MAX_NAME 32
#include "uthash.h"

#define MAX_USERS 1024
struct bufferevent;
struct table_s;

#define USER_STATE_LOGIN     0x01
#define USER_STATE_TABLE     0x02

#define USER_TYPE_TELNET     0x00
#define USER_TYPE_WEBSOCKET  0x01

typedef unsigned int user_state_t;

typedef struct user_s {
    char name[MAX_NAME];
    char password[20];
    char prompt[MAX_NAME];
    int money;
    struct bufferevent *bev;
    struct table_s *table;
    user_state_t state;
    UT_hash_handle hh;
    int index;
} user_t;

#ifdef TEXAS_ASSERT
#define ASSERT_LOGIN(user)         assert((user)->state & USER_STATE_LOGIN)
#define ASSERT_NOT_LOGIN(user)     assert(!((user)->state & USER_STATE_LOGIN))
#define ASSERT_TABLE(user)         assert((user)->state & USER_STATE_TABLE)
#define ASSERT_NOT_TABLE(user)     assert(!((user)->state & USER_STATE_TABLE))
#else
#define ASSERT_LOGIN(user)
#define ASSERT_NOT_LOGIN(user)
#define ASSERT_TABLE(user)
#define ASSERT_NOT_TABLE(user)
#endif

#define CHECK_LOGIN(user) do {\
    if (!((user)->state & USER_STATE_LOGIN)) {\
        send_msg((user), "you are not logged in");\
        return -1;\
    }\
} while(0)

#define CHECK_NOT_LOGIN(user) do {\
    if ((user)->state & USER_STATE_LOGIN) {\
        send_msg((user), "you are already logged in as %s", (user)->name);\
        return -1;\
    }\
} while(0)

#define CHECK_TABLE(user) do {\
    if (!((user)->state & USER_STATE_TABLE)) {\
        send_msg((user), "you are not in table");\
        return -1;\
    }\
} while(0)

#define CHECK_NOT_TABLE(user) do {\
    if ((user)->state & USER_STATE_TABLE) {\
        send_msg((user), "you are already in table");;\
        return -1;\
    }\
} while(0)

extern user_t *g_current_user;
extern user_t *g_users;
extern int g_num_user;
user_t *user_create();
void user_destroy(user_t *user);
void send_msg(user_t *user, const char *fmt, ...);
void send_msg_raw(user_t *user, const char *fmt, ...);
void send_msgv(user_t *user, const char *fmt, va_list ap);
void send_msgv_raw(user_t *user, const char *fmt, va_list ap);
void broadcast_global(const char *fmt, ...);
void broadcast_globalv(const char *fmt, va_list ap);
void broadcast_global_raw(const char *fmt, ...);
void broadcast_globalv_raw(const char *fmt, va_list ap);
void broadcast_global_websocket(const char *fmt, ...);
void broadcast_globalv_websocket(const char *fmt, va_list ap);
int user_save(user_t *user);
int user_load(const char *name, user_t *user);
int user_save_password(user_t *user);
int user_save_prompt(user_t *user);
int user_save_money(user_t *user);

#endif
