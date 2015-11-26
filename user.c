#include "holdem.h"
#include "user.h"
user_t *g_current_user;
user_t *g_users = NULL;
int g_num_users = 0;

user_t *user_create()
{
    user_t *user;

    if (g_num_users >= MAX_USERS) {
        return NULL;
    }
    user = (user_t *)malloc(sizeof(user_t));
    if (user == NULL) {
        return NULL;
    }
    memset(user, 0, sizeof(user_t));
    snprintf(user->prompt, sizeof(user->prompt), ">");
    g_num_users++;
    return user;
}

void user_destroy(user_t *user)
{
    if (user) {
        g_num_users--;
        free(user);
    }
}

void send_msg(user_t *user, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    send_msgv(user, fmt, ap);
    va_end(ap);
}

void send_msgv(user_t *user, const char *fmt, va_list ap)
{
    evbuffer_add_vprintf(bufferevent_get_output(user->bev), fmt, ap);
    evbuffer_add_printf(bufferevent_get_output(user->bev), "%s\n", user->prompt);
}

void send_msg_raw(user_t *user, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    send_msgv_raw(user, fmt, ap);
    va_end(ap);
}

void send_msgv_raw(user_t *user, const char *fmt, va_list ap)
{
    evbuffer_add_vprintf(bufferevent_get_output(user->bev), fmt, ap);
}

void broadcast_global(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    broadcast_globalv(fmt, ap);
    va_end(ap);
}

void broadcast_globalv(const char *fmt, va_list ap)
{
    user_t *tmp, *user;
    va_list cp;

    HASH_ITER(hh, g_users, user, tmp) {
        va_copy(cp, ap);
        send_msgv(user, fmt, cp);
    }
}

void broadcast_global_raw(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    broadcast_globalv_raw(fmt, ap);
    va_end(ap);
}

void broadcast_globalv_raw(const char *fmt, va_list ap)
{
    user_t *tmp, *user;
    va_list cp;

    HASH_ITER(hh, g_users, user, tmp) {
        va_copy(cp, ap);
        send_msgv_raw(user, fmt, cp);
    }
}

int user_save(user_t *user)
{
    int ret;
    ret = user_save_password(user);
    if (ret < 0) {
        return ret;
    }
    ret = user_save_prompt(user);
    if (ret < 0) {
        return ret;
    }
    ret = user_save_money(user);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

int user_load(const char *name, user_t *user)
{
    char bufk[128], bufv[128];
    size_t lenk, lenv;
    int ret;

    lenk = snprintf(bufk, sizeof(bufk), "%s_password", name);
    ret = holdem_db_get(bufk, lenk, bufv, &lenv);
    if (ret < 0) {
        return ret;
    }
    memcpy(user->password, bufv, lenv);

    lenk = snprintf(bufk, sizeof(bufk), "%s_prompt", name);
    ret = holdem_db_get(bufk, lenk, bufv, &lenv);
    if (ret < 0) {
        return ret;
    }
    memcpy(user->prompt, bufv, lenv);

    lenk = snprintf(bufk, sizeof(bufk), "%s_money", name);
    ret = holdem_db_get(bufk, lenk, bufv, &lenv);
    if (ret < 0) {
        return ret;
    }
    memcpy(&(user->money), bufv, lenv);
    strncpy(user->name, name, sizeof(user->name));

    return 0;
}

int user_save_password(user_t *user)
{
    char bufk[128], bufv[128];
    size_t lenk, lenv;
    int ret;

    lenk = snprintf(bufk, sizeof(bufk), "%s_password", user->name);
    lenv = sizeof(user->password);
    memcpy(bufv, user->password, lenv);
    ret = holdem_db_put(bufk, lenk, bufv, lenv);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

int user_save_prompt(user_t *user)
{
    char bufk[128], bufv[128];
    size_t lenk, lenv;
    int ret;

    lenk = snprintf(bufk, sizeof(bufk), "%s_prompt", user->name);
    lenv = sizeof(user->prompt);
    memcpy(bufv, user->prompt, lenv);
    ret = holdem_db_put(bufk, lenk, bufv, lenv);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

int user_save_money(user_t *user)
{
    char bufk[128], bufv[128];
    size_t lenk, lenv;
    int ret;

    lenk = snprintf(bufk, sizeof(bufk), "%s_money", user->name);
    lenv = sizeof(user->money);
    memcpy(bufv, &(user->money), lenv);
    ret = holdem_db_put(bufk, lenk, bufv, lenv);
    if (ret < 0) {
        return ret;
    }
    return 0;
}
