#include "texas_holdem.h"
#include "card.h"
#include "user.h"
#include "table.h"
#include "list.h"
#include "handler.h"

#define MAX_LINE 4096

void do_read(evutil_socket_t fd, short events, void *arg);
void do_write(evutil_socket_t fd, short events, void *arg);

int handle(user_t *user, const char *line)
{
    char line_buffer[MAX_LINE];

    g_current_user = user;
    snprintf(line_buffer, sizeof(line_buffer), "%s\n", line);
    // bin log
    printf("%s: %s\n", user->name, line);
    yy_scan_string(line_buffer);
    yyparse();
    //yylex_destroy();
    return 0;
}

void readcb(struct bufferevent *bev, void *ctx)
{
    user_t *user = (user_t *)ctx;
    char *line;
    size_t n;

    assert(user);
    while ((line = evbuffer_readln(bufferevent_get_input(bev), &n, EVBUFFER_EOL_CRLF))) {
        handle(user, line);
    }
}

void errorcb(struct bufferevent *bev, short error, void *ctx)
{
    user_t *user = (user_t *)ctx;

    assert(user);
    if (error & BEV_EVENT_EOF) {
        /* connection has been closed, do any clean up here */
        /* ... */
    } else if (error & BEV_EVENT_ERROR) {
        /* check errno to see what error occurred */
        /* ... */
    } else if (error & BEV_EVENT_TIMEOUT) {
        /* must be a timeout event handle, handle it */
        /* ... */
    }

    handle(user, "exit");
}

void timeoutcb(evutil_socket_t fd, short events, void *arg)
{
    table_t *table = (table_t *)arg;

    handle_action(table, table->turn, ACTION_FOLD, 0);
}

void do_accept(evutil_socket_t listener, short event, void *arg)
{
    struct event_base *base = arg;
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(listener, (struct sockaddr*)&ss, &slen);
    user_t *user;
    if (fd < 0) {
        perror("accept");
    } else if (fd > FD_SETSIZE) {
        close(fd);
    } else {
        struct bufferevent *bev;
        evutil_make_socket_nonblocking(fd);
        user = user_create();
        if (user == NULL) {
            close(fd);
            return;
        }
        bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        user->bev = bev;
        bufferevent_setcb(bev, readcb, NULL, errorcb, user);
        bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
        bufferevent_enable(bev, EV_READ|EV_WRITE);
    }
}

void run(void)
{
    evutil_socket_t listener;
    struct sockaddr_in sin;
    struct event_base *base;
    struct event *listener_event;

    assert(base = event_base_new());
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(10000);

    listener = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(listener);

#ifndef WIN32
    {
        int one = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    }
#endif

    if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        perror("bind");
        return;
    }

    if (listen(listener, 16)<0) {
        perror("listen");
        return;
    }

    listener_event = event_new(base, listener, EV_READ|EV_PERSIST, do_accept, (void*)base);
    /*XXX check it */
    event_add(listener_event, NULL);

    event_base_dispatch(base);
}

#ifndef UNIT_TEST
int main(int c, char **v)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGPIPE, SIG_IGN);
    texas_db_init();

    run();
    return 0;
}
#endif
