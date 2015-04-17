#include "texas_holdem.h"
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
    snprintf(user->prompt, sizeof(user->prompt), "\n>");
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
        evbuffer_add_printf(bufferevent_get_output(user->bev), "%s", user->prompt);
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

void send_msg_new(user_t *user, const char *fmt1, const *fmt2, ...)
{    
    va_list ap;
    va_start(ap, fmt2);
    send_msgv_new(user, fmt1, fmt2, ap);
    va_end(ap, fmt2);
}

void send_msgv_new(user_t *user, const char *fmt1, const char *fmt2, va_list ap)
{    
    if (user->type == USER_TYPE_TELNET) {
        evbuffer_add_vprintf(bufferevent_get_output(user->bev), fmt1, ap);
        evbuffer_add_printf(bufferevent_get_output(user->bev), "%s", user->prompt);
    } else if (user->type == USER_TYPE_WEBSOCKET) {
        evbuffer_add_vprintf(bufferevent_get_output(user->bev), fmt2, ap);
    }
}

int user_save(user_t *user)
{
    char buffer[512];
    size_t offset = 0, len;

    len = sizeof(user->name);
    memcpy(buffer + offset, user->name, len);
    offset += len;

    len = sizeof(user->password);
    memcpy(buffer + offset, user->password, len);
    offset += len;

    len = sizeof(user->prompt);
    memcpy(buffer + offset, user->prompt, len);
    offset += len;

    len = sizeof(user->money);
    memcpy(buffer + offset, &(user->money), len);
    offset += len;

    return texas_db_put(user->name, strlen(user->name), buffer, sizeof(buffer));
}

int user_load(const char *name, user_t *user)
{
    char buffer[512];
    size_t offset = 0, len;
    int ret;

    ret = texas_db_get(name, strlen(name), buffer, &len);
    if (ret < 0 || len != sizeof(buffer)) {
        return ret;
    }

    len = sizeof(user->name);
    memcpy(user->name, buffer + offset, len);
    offset += len;

    len = sizeof(user->password);
    memcpy(user->password, buffer + offset, len);
    offset += len;

    len = sizeof(user->prompt);
    memcpy(user->prompt, buffer + offset, len);
    offset += len;

    len = sizeof(user->money);
    memcpy(&(user->money), buffer + offset, len);
    offset += len;
    return 0;
}
