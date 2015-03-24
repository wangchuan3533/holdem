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

#define MAX_LINE 16384
table_t g_table;
int num_players = 0;

void do_read(evutil_socket_t fd, short events, void *arg);
void do_write(evutil_socket_t fd, short events, void *arg);
void start_game();


void readcb(struct bufferevent *bev, void *ctx)
{
    struct evbuffer *input;
    player_t *player = (player_t *)ctx, *next;
    table_t *table = &g_table;
    char *line;
    size_t n;
    int bid;
    input = bufferevent_get_input(bev);

    while ((line = evbuffer_readln(input, &n, EVBUFFER_EOL_LF))) {
        if (player == current_player(table) && player->state == PLAYER_STATE_GAME) {
            next = next_player(table);
            switch (line[0]) {
            // raise
            case 'r':
            case 'R':
                bid = atoi(line + 2);
                player->pot -= bid;
                player->bid += bid;
                table->bid  += bid;
                move_next(table);
                broadcast(table, "[PLAYER]%s [RAISE]: %d => %d\n", player->name, bid, player->bid);
                goto _report;
            // call
            case 'c':
            case 'C':
                bid = table->bid - player->bid;
                player->pot -= bid;
                player->bid  += bid;
                broadcast(table, "[PLAYER]%s [CALL]: %d => %d\n", player->name, bid, player->bid);
                if (next->bid == table->bid) {
                    switch (table->state) {
                    case TABLE_STATE_WAITING:
                        table_pre_flop(table);
                        break;
                    case TABLE_STATE_PREFLOP:
                        table_flop(table);
                        break;
                    case TABLE_STATE_FLOP:
                        table_turn(table);
                        break;
                    case TABLE_STATE_TURN:
                        table_river(table);
                        break;
                    case TABLE_STATE_RIVER:
                        table_showdown(table);
                        break;
                    default:
                        broadcast(table, "[GAME] GAME OVER\n");
                        broadcast(table, "[GAME] final is %d\n", table->pot);
                        start_game();
                        break;
                    }
                    break;
                }
                move_next(table);
                goto _report;
            // fold
            case 'f':
                break;
                list_remove(&table->gaming_players, table->turn, NULL);
                list_insert_next(&table->folded_playsers, table->folded_playsers.tail, player);
                player->state = PLAYER_STATE_FOLDED;
                table->pot += player->bid;
                player->bid = 0;
                broadcast(table, "[PLAYER]%s [FOLDS]: %d => %d\n", player->name, bid, player->bid);
                broadcast(table, "[GAME] NEXT IS %s\n", next->name);
                break;
            default:
_report:
                report(table, player);
                break;
_chat:
                broadcast(table, "[CHAT]: %s => %s\n", player->name, line);
                break;
            }
        } else {
            broadcast(table, "[CHAT]: %s => %s\n", player->name, line);
        }
        free(line);
    }
}

void errorcb(struct bufferevent *bev, short error, void *ctx)
{
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
    } else if (g_table.state != TABLE_STATE_WAITING) {
        close(fd);
    } else {
        struct bufferevent *bev;
        evutil_make_socket_nonblocking(fd);
        bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        player = (player_t *)malloc(sizeof(player_t));
        if (player == NULL) {
            perror("malloc");
        }
        snprintf(player->name, sizeof(player->name), "%d", num_players);
        player->bev = bev;
        player->bid = 0;
        player->pot = 10000;
        player->state = PLAYER_STATE_GAME;
        list_insert_next(&g_table.gaming_players, g_table.gaming_players.tail, player);
        bufferevent_setcb(bev, readcb, NULL, errorcb, player);
        bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
        bufferevent_enable(bev, EV_READ|EV_WRITE);
        broadcast(&g_table, "[TABLE] Player %s joined\n", player->name);
        num_players++;
        if (num_players >= MIN_PLAYERS) {
            start_game();
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

void start_game()
{
    table_pre_flop(&g_table);
    broadcast(&g_table, "[GAME] GAME BEGIN\n");
    broadcast(&g_table, "[GAME] Next is %s\n", current_player(&g_table)->name);
}

int main(int c, char **v)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    table_init(&g_table);

    run();
    return 0;
}
