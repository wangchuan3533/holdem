#include <string.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include "table.h"
void table_init(table_t *table)
{
    list_init(&table->folded_playsers, NULL);
    list_init(&table->waiting_players, NULL);
    list_init(&table->gaming_players, NULL);
}

void table_pre_flop(table_t *table)
{
    list_node_t *iter;
    player_t *player;

    init_deck(&(table->deck));
    for (iter = table->gaming_players.head; iter != NULL;  iter = iter->next) {
        player = (player_t *)iter->data;
        table->pot += player->bid;
        player->bid = 0;
        player->hand_cards[0] = get_card(&table->deck);
        player->hand_cards[1] = get_card(&table->deck);
    }
    table->state = TABLE_STATE_PREFLOP;
    table->pot = 0;
    table->bid  = 0;
    table->turn = table->gaming_players.head;
}

void table_flop(table_t *table)
{
    list_node_t *iter;
    player_t *player;

    table->community_cards[0] = get_card(&table->deck);
    table->community_cards[1] = get_card(&table->deck);
    table->community_cards[2] = get_card(&table->deck);

    for (iter = table->gaming_players.head; iter != NULL;  iter = iter->next) {
        player = (player_t *)iter->data;
        table->pot += player->bid;
        player->bid = 0;
        player->hand_cards[2] = table->community_cards[0];
        player->hand_cards[3] = table->community_cards[1];
        player->hand_cards[4] = table->community_cards[2];
    }
    table->state = TABLE_STATE_FLOP;
    table->bid  = 0;
    table->turn = table->gaming_players.head;
}

void table_turn(table_t *table)
{
    list_node_t *iter;
    player_t *player;

    table->community_cards[3] = get_card(&table->deck);

    for (iter = table->gaming_players.head; iter != NULL;  iter = iter->next) {
        player = (player_t *)iter->data;
        table->pot += player->bid;
        player->bid = 0;
        player->hand_cards[5] = table->community_cards[3];
    }
    table->state = TABLE_STATE_TURN;
    table->bid  = 0;
    table->turn = table->gaming_players.head;
}

void table_river(table_t *table)
{
    list_node_t *iter;
    player_t *player;

    table->community_cards[4] = get_card(&table->deck);

    for (iter = table->gaming_players.head; iter != NULL;  iter = iter->next) {
        player = (player_t *)iter->data;
        table->pot += player->bid;
        player->bid = 0;
        player->hand_cards[6] = table->community_cards[4];
    }
    table->state = TABLE_STATE_RIVER;
    table->bid  = 0;
    table->turn = table->gaming_players.head;
}

void table_showdown(table_t *table)
{
    list_node_t *iter;
    player_t *player, *winner;
    card_t max = 0;

    for (iter = table->gaming_players.head; iter != NULL;  iter = iter->next) {
        player = (player_t *)iter->data;
        table->pot += player->bid;
        player->bid = 0;
        if (player->hand_cards[0] > max) {
            max = player->hand_cards[0];
            winner = player;
        }
        player->hand_cards[6] = table->community_cards[4];
    }
    table->state = TABLE_STATE_RIVER;
    table->bid  = 0;
    table->turn = table->gaming_players.head;
}

void broadcast(table_t *table, const char *fmt, ...)
{
    va_list ap;
    list_node_t *iter;
    player_t *player;
    va_start(ap, fmt);
    for (iter = table->gaming_players.head; iter != NULL; iter = iter->next) {
        player = (player_t *)iter->data;
        evbuffer_add_vprintf(bufferevent_get_output(player->bev), fmt, ap);
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


