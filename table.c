#include "texas_holdem.h"
#include "table.h"

char g_table_report_buffer[4096];
table_t *g_tables = NULL;
int g_num_tables = 0;
static struct timeval _timeout = {60, 0};

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

void table_reset(table_t *table)
{
    int i;

    table->state = TABLE_STATE_WAITING;
    init_deck(&(table->deck));
    table->pot           = 0;
    table->bet           = 0;
    table->small_blind   = 50;
    table->big_blind     = 100;
    table->minimum_bet   = 100;
    table->minimum_raise = 100;
    table->raise_count   = 0;
    table->num_playing   = 0;
    table_clear_timeout(table);

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]) {
            ASSERT_LOGIN(table->players[i]);
            ASSERT_TABLE(table->players[i]);
            table->players[i]->state &= ~PLAYER_STATE_GAME;
            table->players[i]->bet = 0;
            table->players[i]->rank.level = 0;
            table->players[i]->rank.score = 0;
        }
    }
}

void table_pre_flop(table_t *table)
{
    int i;

    if (table->num_players < MIN_PLAYERS) {
        return;
    }

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]) {
            ASSERT_LOGIN(table->players[i]);
            ASSERT_TABLE(table->players[i]);
            table->players[i]->state |= PLAYER_STATE_GAME;
            table->players[i]->hand_cards[0] = get_card(&table->deck);
            table->players[i]->hand_cards[1] = get_card(&table->deck);
            table->num_playing++;
            //send_msg(table->players[i], "[\"pre_flop\",[\"%s\",\"%s\"]]\n",
            send_msg(table->players[i], "[PRE_FLOP] %s, %s",
                    card_to_string(table->players[i]->hand_cards[0]),
                    card_to_string(table->players[i]->hand_cards[1]));
        }
    }

    table->state = TABLE_STATE_PREFLOP;
    table->dealer = next_player(table, table->dealer);
    table->turn = next_player(table, table->dealer);

    // small blind bet
    current_player(table)->bet += table->small_blind;
    current_player(table)->pot -= table->small_blind;
    table->pot += current_player(table)->bet;
    broadcast(table, "%s blind %d", current_player(table)->name, current_player(table)->bet);
    table->turn = next_player(table, table->turn);

    // big blind bet
    current_player(table)->bet += table->big_blind;
    current_player(table)->pot -= table->big_blind;
    table->pot += current_player(table)->bet;
    broadcast(table, "%s blind %d", current_player(table)->name, current_player(table)->bet);
    table->turn = next_player(table, table->turn);
    table_reset_timeout(table);

    table->bet = table->big_blind;
    report(table);
}

void table_flop(table_t *table)
{
    int i;

    table->community_cards[0] = get_card(&table->deck);
    table->community_cards[1] = get_card(&table->deck);
    table->community_cards[2] = get_card(&table->deck);

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]) {
            ASSERT_LOGIN(table->players[i]);
            ASSERT_TABLE(table->players[i]);
            if (table->players[i]->state & PLAYER_STATE_GAME) {
                table->players[i]->bet = 0;
                table->players[i]->hand_cards[2] = table->community_cards[0];
                table->players[i]->hand_cards[3] = table->community_cards[1];
                table->players[i]->hand_cards[4] = table->community_cards[2];
            }
        }
    }
    //broadcast(table, "[\"flop\",[\"%s\",\"%s\",\"%s\"]]\n",
    broadcast(table, "[FLOP] %s, %s, %s",
            card_to_string(table->community_cards[0]),
            card_to_string(table->community_cards[1]),
            card_to_string(table->community_cards[2]));
    table->state = TABLE_STATE_FLOP;
    table->bet  = 0;
    table->turn = next_player(table, table->dealer);
    table_reset_timeout(table);
    report(table);
}

void table_turn(table_t *table)
{
    int i;

    table->community_cards[3] = get_card(&table->deck);

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]) {
            ASSERT_LOGIN(table->players[i]);
            ASSERT_TABLE(table->players[i]);
            table->players[i]->bet = 0;
            table->players[i]->hand_cards[5] = table->community_cards[3];
        }
    }
    //broadcast(table, "[\"turn\",[\"%s\"]]\n",card_to_string(table->community_cards[3]));
    broadcast(table, "[TURN] %s", card_to_string(table->community_cards[3]));
    table->state = TABLE_STATE_TURN;
    table->bet  = 0;
    table->turn = next_player(table, table->dealer);
    table_reset_timeout(table);
    report(table);
}

void table_river(table_t *table)
{
    int i;

    table->community_cards[4] = get_card(&table->deck);

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]) {
            ASSERT_LOGIN(table->players[i]);
            ASSERT_TABLE(table->players[i]);
            table->players[i]->bet = 0;
            table->players[i]->hand_cards[6] = table->community_cards[4];
        }
    }
    //broadcast(table, "[\"river\",[\"%s\"]]\n", card_to_string(table->community_cards[3]));
    broadcast(table, "[RIVER] %s", card_to_string(table->community_cards[4]));
    table->state = TABLE_STATE_RIVER;
    table->bet  = 0;
    table->turn = next_player(table, table->dealer);
    table_reset_timeout(table);
    report(table);
}

void table_showdown(table_t *table)
{
    int i;

    player_t *winner;
    hand_rank_t max = {0, 0, 0};

    broadcast(table, "[SHOWDOWN] community cards is %s, %s, %s, %s, %s",
            card_to_string(table->community_cards[0]),
            card_to_string(table->community_cards[1]),
            card_to_string(table->community_cards[2]),
            card_to_string(table->community_cards[3]),
            card_to_string(table->community_cards[4]));
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]) {
            ASSERT_LOGIN(table->players[i]);
            ASSERT_TABLE(table->players[i]);
            table->players[i]->rank = calc_rank(table->players[i]->hand_cards);
            broadcast(table, "player %s's cards is [%s, %s]. rank is [%s], mask is %#x, score is %d",
                    table->players[i]->name, card_to_string(table->players[i]->hand_cards[0]),
                    card_to_string(table->players[i]->hand_cards[1]), level_to_string(table->players[i]->rank.level),
                    table->players[i]->rank.mask, table->players[i]->rank.score);
            if (rank_cmp(table->players[i]->rank, max) > 0) {
                max = table->players[i]->rank;
                winner = table->players[i];
            }
        }
    }

    //broadcast(table, "[\"finish\",{\"winner\":\"%s\",\"pot\":%d}]\n", winner->name, table->pot);
    broadcast(table, "[WINNER] %s [POT] %d", winner->name, table->pot);
    winner->pot += table->pot;
    table->pot = 0;
    table_reset(table);
    handle_table(table);
}

int table_check_winner(table_t *table)
{
    int i;

    assert(table->num_playing == 1);
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i] && (table->players[i]->state & PLAYER_STATE_GAME)) {
            ASSERT_LOGIN(table->players[i]);
            ASSERT_TABLE(table->players[i]);
            //broadcast(table, "[\"finish\",{\"winner\":\"%s\",\"pot\":%d}]\n", table->players[i]->name, table->pot);
            broadcast(table, "[WINNER] %s [POT] %d", table->players[i]->name, table->pot);
            table->players[i]->pot += table->pot;
            table_reset(table);
            return TEXAS_RET_SUCCESS;
        }
    }
    return TEXAS_RET_FAILURE;
}

inline void table_init_timeout(table_t *table)
{
    table->ev_timeout = event_new(table->base, -1, 0, timeoutcb, table);
}

inline void table_reset_timeout(table_t *table)
{
    event_add(table->ev_timeout, &_timeout);
}

inline void table_clear_timeout(table_t *table)
{
    if (table->ev_timeout) {
        event_del(table->ev_timeout);
    }
    table->ev_timeout = NULL;
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
            "{\"community_cards\":[%s],\"players\":[%s],\"dealer\":%d,\"turn\":\"%s\",\"pot\":%d,\"bet\":%d,\"minimum_bet\":%d}",
            cards_buffer, players_buffer, table->dealer, current_player(table)->name, table->pot, table->bet, table->minimum_bet);
}

void broadcast(table_t *table, const char *fmt, ...)
{
    int i;

    va_list ap;
    va_start(ap, fmt);
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]) {
            ASSERT_LOGIN(table->players[i]);
            ASSERT_TABLE(table->players[i]);
            send_msgv(table->players[i], fmt, ap);
        }
    }
    va_end(ap);
}

int player_join(table_t *table, player_t *player)
{
    int i;

    ASSERT_LOGIN(player);
    ASSERT_NOT_TABLE(player);
    if (table->num_players >= TABLE_MAX_PLAYERS) {
        return TEXAS_RET_MAX_PLAYER;
    }
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i] == NULL) {
            table->players[i] = player;
            player->table = table;
            player->state |= PLAYER_STATE_TABLE;
            table->num_players++;
            //broadcast(table, "[\"join\",{\"name\":\"%s\"}]\n", player->name);
            broadcast(table, "%s joined", player->name);
            return TEXAS_RET_SUCCESS;
        }
    }
    assert(0);
    return TEXAS_RET_FAILURE;
}

int player_quit(player_t *player)
{
    int i;

    ASSERT_LOGIN(player);
    ASSERT_TABLE(player);
    ASSERT_NOT_GAME(player);

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (player->table->players[i] == player) {
            player->table->num_players--;
            player->table->players[i] = NULL;
            player->table = NULL;
            player->state &= ~PLAYER_STATE_TABLE;
            return TEXAS_RET_SUCCESS;
        }
    }
    // fatal
    assert(0);
    return TEXAS_RET_FAILURE;
}

int next_player(table_t *table, int index)
{
    int i, next;

    for (i = 1; i < TABLE_MAX_PLAYERS; i++) {
        next = (index + i) % TABLE_MAX_PLAYERS;
        if (table->players[next] && (table->players[next]->state & PLAYER_STATE_GAME)) {
            return next;
        }
    }

    return -1;
}

int player_bet(player_t *player, int bet)
{
    table_t *table = player->table;

    ASSERT_LOGIN(player);
    ASSERT_TABLE(player);
    ASSERT_GAME(player);
    assert(player->bet == 0);
    assert(table->bet == 0);

    if (bet > player->pot) {
        return TEXAS_RET_LOW_POT;
    }
    if (bet < table->minimum_bet) {
        return TEXAS_RET_MIN_BET;
    }
    player->pot -= bet;
    table->pot += bet;
    player->bet += bet;
    table->bet = player->bet;
    table->minimum_raise = table->bet;

    return TEXAS_RET_SUCCESS;
}

int player_raise(player_t *player, int raise)
{
    table_t *table = player->table;
    int diff = table->bet - player->bet;

    ASSERT_LOGIN(player);
    ASSERT_TABLE(player);
    ASSERT_GAME(player);
    assert(player->pot >= diff + table->minimum_raise);

    if (raise < table->minimum_raise) {
        return TEXAS_RET_MIN_BET;
    }
    if (raise + diff > player->pot) {
        return TEXAS_RET_LOW_POT;
    }
    player->pot -= (raise + diff);
    table->pot += (raise + diff);
    player->bet += (raise + diff);
    table->bet = player->bet;
    table->raise_count++;
    table->minimum_raise = raise;

    return TEXAS_RET_SUCCESS;
}

int player_call(player_t *player)
{
    table_t *table = player->table;
    int diff = player->table->bet - player->bet;

    ASSERT_LOGIN(player);
    ASSERT_TABLE(player);
    ASSERT_GAME(player);
    assert(player->pot >= diff);

    player->pot -= diff;
    player->table->pot += diff;
    player->bet += diff;

    return TEXAS_RET_SUCCESS;
}

int player_fold(player_t *player)
{
    table_t *table = player->table;

    ASSERT_LOGIN(player);
    ASSERT_TABLE(player);
    ASSERT_GAME(player);

    player->state &= ~PLAYER_STATE_GAME;
    player->bet = 0;
    player->table->num_playing--;
    return TEXAS_RET_SUCCESS;
}

int player_check(player_t *player)
{
    table_t *table = player->table;

    ASSERT_LOGIN(player);
    ASSERT_TABLE(player);
    ASSERT_GAME(player);
    assert(player->bet == 0);
    assert(player->table->bet == 0);

    return TEXAS_RET_SUCCESS;
}

int handle_action(player_t *player, action_t action)
{
    table_t *table = player->table;
    player_t *player_next;
    int next;

    ASSERT_LOGIN(player);
    ASSERT_TABLE(player);
    ASSERT_GAME(player);

    // check action
    if (!(action.action & table->action_mask)) {
        return TEXAS_RET_ACTION;
    }

    if (action.action != ACTION_FOLD) {
        CHECK_IN_TURN(player);
    }

    switch (action.action) {
    case ACTION_BET:
        if (player_bet(player, action.value) != TEXAS_RET_SUCCESS) {
            send_msg("bet failed");
            return TEXAS_RET_FAILURE;
        }
        break;
    case ACTION_RAISE:
        if (player_raise(player, action.value) != TEXAS_RET_SUCCESS) {
            send_msg("raise failed");
            return TEXAS_RET_FAILURE;
        }
        break;
    case ACTION_CALL:
        if (player_call(player) != TEXAS_RET_SUCCESS) {
            send_msg("call failed");
            return TEXAS_RET_FAILURE;
        }
        break;
    case ACTION_CHCEK:
        if (player_check(player) != TEXAS_RET_SUCCESS) {
            send_msg("check failed");
            return TEXAS_RET_FAILURE;
        }
        break;
    case ACTION_ALL_IN:
        if (player_all_in(player) != TEXAS_RET_SUCCESS) {
            send_msg("check failed");
            return TEXAS_RET_FAILURE;
        }
        break;
    case ACTION_FOLD:
        if (player_fold(player) != TEXAS_RET_SUCCESS) {
            send_msg("check failed");
            return TEXAS_RET_FAILURE;
        }
        break;
    default:
        err_quit("default");
        break;
    }

    if (table->num_playing == 1) {
        assert(table_check_winner(player->table) == 0);
        handle_table(player->table);
        return TEXAS_RET_SUCCESS;
    }

    if (current_player(table) == player) {
        if (table->state == TABLE_STATE_PREFLOP && table->turn == next_player(table, next_player(
        
        // generate available actions
        next = next_player(table, table->turn);
        player_next = table->players[next];
        assert(next >= 0);
        assert(player_next);

        table->action_mask = 0;
        table->action_mask |= ACTION_FOLD;

        if (player_next->pot > 0) {
            table->action_mask |= ACTION_ALL_IN;
        }

        if (table->bet == 0) {
            table->action_mask |= ACTION_CHCEK;
            table->action_mask |= ACTION_BET;
        }

        if (player_next->pot + player_next->bet >= table->bet) {
            table->action_mask |= ACTION_CALL;
        }

        if (player_next->pot + player_next->bet >= table->bet + table->minimum_raise) {
            table->action_mask |= ACTION_RAISE;
        }

    }
    return TEXAS_RET_SUCCESS;
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
    return TEXAS_RET_SUCCESS;
}


