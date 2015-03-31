#include <string.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <stdlib.h>
#include "table.h"

char g_table_report_buffer[4096];
table_t *g_tables = NULL;
int g_num_tables = 0;

table_t *table_create()
{
    table_t *table;

    if (g_num_tables >= MAX_TABLES) {
        return NULL;
    }
    table = (table_t *)malloc(sizeof(table_t));
    if (table == NULL) {
        return NULL;
    }

    memset(table, 0, sizeof(table_t));
    table->small_blind = 50;
    table->big_blind   = 100;
    table->minimum_bet = 100;
    table->num_playing = 0;
    g_num_tables++;
    return table;
}

void table_destroy(table_t *table)
{
    if (table) {
        free(table);
        g_num_tables--;
    }
}

void table_pre_flop(table_t *table)
{
    int i;

    if (table->num_players < MIN_PLAYERS) {
        table->state = TABLE_STATE_WAITING;
        return;
    }

    init_deck(&(table->deck));
    for (i = 0, table->num_playing = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]
                && (table->players[i]->state == PLAYER_STATE_FOLDED
                    || table->players[i]->state == PLAYER_STATE_GAME
                    || table->players[i]->state == PLAYER_STATE_WAITING)) {
            table->players[i]->state = PLAYER_STATE_GAME;
            table->players[i]->hand_cards[0] = get_card(&table->deck);
            table->players[i]->hand_cards[1] = get_card(&table->deck);
            table->players[i]->bid = 0;
            table->players[i]->rank.level = 0;
            table->players[i]->rank.score = 0;
            table->num_playing++;
            //send_msg(table->players[i], "[\"pre_flop\",[\"%s\",\"%s\"]]\n",
            send_msg(table->players[i], "[pre_flop] [%s, %s]\ntexas> ",
                    card_to_string(table->players[i]->hand_cards[0]),
                    card_to_string(table->players[i]->hand_cards[1]));
        }
    }

    table->state = TABLE_STATE_PREFLOP;
    table->pot = 0;
    table->bid = 0;
    table->dealer = next_player(table, table->dealer);
    table->turn = next_player(table, table->dealer);

    // small blind bet
    table->players[table->turn]->bid += table->small_blind;
    table->players[table->turn]->pot -= table->small_blind;
    table->pot += table->players[table->turn]->bid;
    broadcast(table, "%s blind %d\ntexas> ", table->players[table->turn]->name, table->players[table->turn]->bid);
    table->turn = next_player(table, table->turn);

    // big blind bet
    table->players[table->turn]->bid += table->big_blind;
    table->players[table->turn]->pot -= table->big_blind;
    table->pot += table->players[table->turn]->bid;
    broadcast(table, "%s blind %d\ntexas> ", table->players[table->turn]->name, table->players[table->turn]->bid);
    table->turn = next_player(table, table->turn);

    table->bid = table->big_blind;
    report(table);
}

void table_flop(table_t *table)
{
    int i;

    table->community_cards[0] = get_card(&table->deck);
    table->community_cards[1] = get_card(&table->deck);
    table->community_cards[2] = get_card(&table->deck);

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i] && table->players[i]->state == PLAYER_STATE_GAME) {
            table->players[i]->bid = 0;
            table->players[i]->hand_cards[2] = table->community_cards[0];
            table->players[i]->hand_cards[3] = table->community_cards[1];
            table->players[i]->hand_cards[4] = table->community_cards[2];
        }
    }
    //broadcast(table, "[\"flop\",[\"%s\",\"%s\",\"%s\"]]\n",
    broadcast(table, "[flop] [%s,%s,%s]\ntexas> ",
            card_to_string(table->community_cards[0]),
            card_to_string(table->community_cards[1]),
            card_to_string(table->community_cards[2]));
    table->state = TABLE_STATE_FLOP;
    table->bid  = 0;
    table->turn = next_player(table, table->dealer);
    report(table);
}

void table_turn(table_t *table)
{
    int i;

    table->community_cards[3] = get_card(&table->deck);

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i] && table->players[i]->state == PLAYER_STATE_GAME) {
            table->players[i]->bid = 0;
            table->players[i]->hand_cards[5] = table->community_cards[3];
        }
    }
    //broadcast(table, "[\"turn\",[\"%s\"]]\n",card_to_string(table->community_cards[3]));
    broadcast(table, "[turn] [%s]\ntexas> ",card_to_string(table->community_cards[3]));
    table->state = TABLE_STATE_TURN;
    table->bid  = 0;
    table->turn = next_player(table, table->dealer);
    report(table);
}

void table_river(table_t *table)
{
    int i;

    table->community_cards[4] = get_card(&table->deck);

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i] && table->players[i]->state == PLAYER_STATE_GAME) {
            table->players[i]->bid = 0;
            table->players[i]->hand_cards[6] = table->community_cards[4];
        }
    }
    //broadcast(table, "[\"river\",[\"%s\"]]\n",card_to_string(table->community_cards[3]));
    broadcast(table, "[river] [%s]\ntexas> ",card_to_string(table->community_cards[4]));
    table->state = TABLE_STATE_RIVER;
    table->bid  = 0;
    table->turn = next_player(table, table->dealer);
    report(table);
}

void table_showdown(table_t *table)
{
    int i;

    player_t *winner;
    hand_rank_t max = {0, 0};

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i] && table->players[i]->state == PLAYER_STATE_GAME) {
            table->players[i]->bid = 0;
            table->players[i]->rank = calc_rank(table->players[i]->hand_cards);
            if (rank_cmp(table->players[i]->rank, max) > 0) {
                max = table->players[i]->rank;
                winner = table->players[i];
            }
        }
    }

    //broadcast(table, "[\"finish\",{\"winner\":\"%s\",\"pot\":%d}]\n", winner->name, table->pot);
    broadcast(table, "[winner] %s [pot] %d\ntexas> ", winner->name, table->pot);
    winner->pot += table->pot;
    table->pot = 0;
    table_pre_flop(table);
}

int table_check_winner(table_t *table)
{
    int i;

    if (table->num_playing == 1) {
        for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
            if (table->players[i] && table->players[i]->state == PLAYER_STATE_GAME) {
                //broadcast(table, "[\"finish\",{\"winner\":\"%s\",\"pot\":%d}]\n", table->players[i]->name, table->pot);
                broadcast(table, "[winner] %s [pot] %d\ntexas> ", table->players[i]->name, table->pot);
                table->players[i]->pot += table->pot;
                table->pot = 0;
                table_pre_flop(table);
                return 0;
            }
        }
    }
    return -1;
}

int table_to_json(table_t *table, char *buffer, int size)
{
    char players_buffer[1024], cards_buffer[256];
    int i, offset;

    for (i = 0, offset = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]) {
            offset += player_to_json(table->players[i], players_buffer + offset, sizeof(players_buffer) - offset);
            players_buffer[offset++] = ',';
        }
    }
    players_buffer[--offset] = '\0';

    switch (table->state) {
    case TABLE_STATE_FLOP:
        snprintf(cards_buffer, sizeof(cards_buffer), "%d,%d,%d",
                table->community_cards[0],
                table->community_cards[1],
                table->community_cards[2]);
        break;
    case TABLE_STATE_TURN:
        snprintf(cards_buffer, sizeof(cards_buffer), "%d,%d,%d,%d",
                table->community_cards[0],
                table->community_cards[1],
                table->community_cards[2],
                table->community_cards[3]);
        break;
    case TABLE_STATE_RIVER:
    case TABLE_STATE_SHOWDOWN:
        snprintf(cards_buffer, sizeof(cards_buffer), "%d,%d,%d,%d,%d",
                table->community_cards[0],
                table->community_cards[1],
                table->community_cards[2],
                table->community_cards[3],
                table->community_cards[4]);
        break;
    case TABLE_STATE_PREFLOP:
    default:
        cards_buffer[0] = '\0';
        break;
    }
    return snprintf(buffer, size, 
            "{\"community_cards\":[%s],\"players\":[%s],\"dealer\":%d,\"turn\":\"%s\",\"pot\":%d,\"bid\":%d,\"minimum_bet\":%d}",
            cards_buffer, players_buffer, table->dealer, table->players[table->turn]->name, table->pot, table->bid, table->minimum_bet);
}

void broadcast(table_t *table, const char *fmt, ...)
{
    int i;

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    va_start(ap, fmt);
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]
                && (table->players[i]->state == PLAYER_STATE_FOLDED
                    || table->players[i]->state == PLAYER_STATE_GAME
                    || table->players[i]->state == PLAYER_STATE_WAITING)) {
            evbuffer_add_vprintf(bufferevent_get_output(table->players[i]->bev), fmt, ap);
        }
    }
    va_end(ap);
}

void send_msg(player_t *player, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    evbuffer_add_vprintf(bufferevent_get_output(player->bev), fmt, ap);
    va_end(ap);
}

int add_player(table_t *table, player_t *player)
{
    int i;

    if (table->num_players >= TABLE_MAX_PLAYERS) {
        return -1;
    }
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i] == NULL) {
            table->players[i] = player;
            player->table = table;
            player->state = PLAYER_STATE_WAITING;
            table->num_players++;
            //broadcast(table, "[\"join\",{\"name\":\"%s\"}]\n", player->name);
            broadcast(table, "%s joined\ntexas> ", player->name);
            if (table->num_players >= MIN_PLAYERS && table->state == TABLE_STATE_WAITING) {
                table_pre_flop(table);
            }
            return 0;
        }
    }
    // fatal
    return -1;
}


int del_player(table_t *table, player_t *player)
{
    int i;
    if (player->state == PLAYER_STATE_GAME) {
        player_fold(player);
    }
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i] == player) {
            table->num_players--;
            table->players[i] = NULL;
            player->table = NULL;
            player->state = PLAYER_STATE_LOGIN;
            return 0;
        }
    }
    // fatal
    return -1;
}

int next_player(table_t *table, int index)
{
    int i, next;
    for (i = 1; i < TABLE_MAX_PLAYERS; i++) {
        next = (index + i) % TABLE_MAX_PLAYERS;
        if (table->players[next] && table->players[next]->state == PLAYER_STATE_GAME) {
            return next;
        }
    }

    return -1;
}

int player_bet(player_t *player, int bid)
{
    if (bid + player->bid < player->table->minimum_bet) {
        return -1;
    }
    if (bid < player->table->bid - player->bid) {
        return -2;
    }
    player->pot -= bid;
    player->table->pot += bid;
    player->bid += bid;
    if (player->bid > player->table->bid) {
        player->table->bid = player->bid;
    }

    return 0;
}

int player_fold(player_t *player)
{

    player->state = PLAYER_STATE_FOLDED;
    player->bid = 0;
    player->table->num_playing--;
    return 0;
}

int player_check(player_t *player)
{
    if (player->table->bid != 0) {
        send_msg(player, "check failed\ntexas> ");
        return -1;
    }

    return 0;
}


int handle_table(table_t *table)
{
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
        break;
    }
    return 0;
}
