/* For sockaddr_in */
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
/* For fcntl */
#include <fcntl.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

#include "card.h"
#include "player.h"
#include "table.h"
#include "list.h"

#define err_quit(fmt, args...) do {\
    fprintf(stderr, "[file:%s line:%d]", __FILE__, __LINE__);\
    fprintf(stderr, fmt, ##args);\
    exit(1);\
} while (0)


#define MAX_LINE 16384
table_t g_table;

void do_read(evutil_socket_t fd, short events, void *arg);
void do_write(evutil_socket_t fd, short events, void *arg);

void readcb(struct bufferevent *bev, void *ctx)
{
    struct evbuffer *input;
    player_t *player = (player_t *)ctx;
    table_t *table = player->table;
    char *line;
    size_t n;
    int bid, next;
    input = bufferevent_get_input(bev);

    while ((line = evbuffer_readln(input, &n, EVBUFFER_EOL_LF))) {
        if (player->state == PLAYER_STATE_NAME) {
	    player->state = PLAYER_STATE_WAITING;
	    snprintf(player->name, sizeof(player->name), "%s", line);
       	    if (add_player(&g_table, player) < 0) {
	        bufferevent_free(bev);
	        memset(player, 0, sizeof(player_t));
	    }
	    continue;
	}
        if (player == current_player(table) && player->state == PLAYER_STATE_GAME) {
            next = next_player(table, table->turn);
            if (next < 0) {
                err_quit("next");
            }
            switch (line[0]) {
            // raise
            case 'r':
                bid = atoi(line + 2);
                if (player_bet(player, bid) != 0) {
                    goto _chat;
                }
                table->turn = next;
                goto _report;
            // call
            case 'c':
                bid = table->bid - player->bid;
                if (player_bet(player, bid) != 0) {
                    goto _chat;
                }
                if (table->players[next]->bid == table->bid) {
                    handle_table(table);
                    break;
                }
                table->turn = next;
                goto _report;
            // fold
            case 'f':
                player_fold(player);
                if (table_check_winner(table) < 0) {
                    table->turn = next;
                    goto _report;
                }
                break;
            default:
                goto _chat;
_report:
                report(table);
                break;
            }
        } else {
_chat:
            broadcast(table, "[\"chat\",{\"user\":\"%s\",\"msg\":\"%s\"}]\n", player->name, line);
        }
        free(line);
    }
}

void errorcb(struct bufferevent *bev, short error, void *ctx)
{
    player_t *player = (player_t *)ctx;

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
    del_player(player->table, player);
    memset(player, 0, sizeof(player_t));
    bufferevent_free(bev);
}

void do_accept(evutil_socket_t listener, short event, void *arg)
{
    struct event_base *base = arg;
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(listener, (struct sockaddr*)&ss, &slen);
    player_t *player;
    if (fd < 0) {
        perror("accept");
    } else if (fd > FD_SETSIZE) {
        close(fd);
    } else {
        struct bufferevent *bev;
        evutil_make_socket_nonblocking(fd);
        player = available_player();
	if (player == NULL) {
            close(fd);
	    return;
	}
        bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        player->state = PLAYER_STATE_NAME;
        //snprintf(player->name, sizeof(player->name), "%lx", (uint64_t)player);
        //player->state = PLAYER_STATE_WAITING;
        player->bev = bev;
        player->bid = 0;
        player->pot = 10000;
        bufferevent_setcb(bev, readcb, NULL, errorcb, player);
        bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
        bufferevent_enable(bev, EV_READ|EV_WRITE);
        if (g_table.num_players >= MIN_PLAYERS) {
            table_pre_flop(&g_table);
        }
    }
}

void run(void)
{
    evutil_socket_t listener;
    struct sockaddr_in sin;
    struct event_base *base;
    struct event *listener_event;

    base = event_base_new();
    if (!base)
        return; /*XXXerr*/

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
    table_init(&g_table);

    run();
    return 0;
}
#endif
