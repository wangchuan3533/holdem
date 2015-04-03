#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#define SERVER_PORT 10000

struct event_base *base;
struct event *timmer;
struct sockaddr_in s_in;
static struct timeval timeout = {1, 0};

void timeoutcb(evutil_socket_t fd, short events, void *arg)
{
    struct bufferevent *bev = (struct bufferevent *)arg;
    printf("timer\n");
    evbuffer_add_printf(bufferevent_get_output(bev), "mk x%ld\r\n", random());
    evbuffer_add_printf(bufferevent_get_output(bev), "exit\r\n");
    event_add(timmer, &timeout);
}

void readcb(struct bufferevent *bev, void *ctx)
{
    char *line;
    size_t n;

    while ((line = evbuffer_readln(bufferevent_get_input(bev), &n, EVBUFFER_EOL_CRLF))) {
        printf("%s\n", line);
    }
}

void writecb(struct bufferevent *bev, void *ctx)
{
}

void eventcb(struct bufferevent *bev, short events, void *ctx)
{
    if (events & BEV_EVENT_CONNECTED) {
        printf("connected\n");
    }
}

int main()
{
    struct bufferevent *bev;

    s_in.sin_family = AF_INET;
    s_in.sin_addr.s_addr = 0;
    s_in.sin_port = htons(SERVER_PORT);

    assert(base = event_base_new());

    bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, readcb, writecb, eventcb, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    evbuffer_add_printf(bufferevent_get_output(bev), "login x\r\n");
    if (bufferevent_socket_connect(bev, (struct sockaddr *)&s_in, sizeof s_in) < 0) {
        bufferevent_free(bev);
        printf("connect failed\n");
        exit(1);
    }

    timmer = event_new(base, -1, 0, timeoutcb, bev);
    event_add(timmer, &timeout);

    event_base_dispatch(base);
    return 0;
}
