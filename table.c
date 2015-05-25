#include "texas_holdem.h"
#include "table.h"

table_t *g_tables = NULL;
int g_num_tables = 0;
static struct timeval _timeout_minute = {60, 0};

table_t *table_create()
{
    int i;
    table_t *table;
    player_t *players;

    if (g_num_tables >= MAX_TABLES) {
        return NULL;
    }
    table = (table_t *)malloc(sizeof(table_t));
    if (table == NULL) {
        return NULL;
    }
    memset(table, 0, sizeof(table_t));
    players = (player_t *)malloc(TABLE_MAX_PLAYERS * sizeof(player_t));
    if (players == NULL) {
        return NULL;
    }
    memset(players, 0, TABLE_MAX_PLAYERS * sizeof(player_t));
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        table->players[i] = players + i;
    }
    g_num_tables++;
    return table;
}

void table_destroy(table_t *table)
{
    if (table) {
        if (table->players[0]) {
            free(table->players[0]);
        }
        free(table);
        g_num_tables--;
    }
}

void table_prepare(table_t *table)
{
    int i;

    table->pot           = 0;
    table->bet           = 0;
    table->minimum_bet   = 100;
    table->minimum_raise = 100;
    table->num_cards     = 0;
    table->num_active    = 0;
    table->num_all_in    = 0;
    table->num_folded    = 0;
    table->pot_count     = 0;
    table->turn          = -1;
    table->action_mask   = 0;
    init_deck(&(table->deck));
    table_clear_timeout(table);
    memset(table->side_pots, 0, sizeof(table->side_pots));
    memset(table->community_cards, 0, sizeof(table->community_cards));

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {

        table->players[i]->bet        = 0;
        table->players[i]->talked     = 0;
        table->players[i]->rank.level = 0;
        table->players[i]->rank.score = 0;
        table->players[i]->pot_offset = 0;

        if (table->players[i]->user) {
            if (table->players[i]->chips < table->minimum_bet && table->players[i]->user->money > 10000 ) {
                player_buy_chips(table->players[i], 10000);
            }
            table->players[i]->state = PLAYER_STATE_WAITING;
            if (table->players[i]->chips >= table->minimum_bet) {
                table->num_available++;
            }
        } else {
            table->players[i]->state = PLAYER_STATE_EMPTY;
        }
    }

    table->state  = TABLE_STATE_WAITING;
    table_to_json(table, table->json_cache, sizeof(table->json_cache));
}

void table_pre_flop(table_t *table)
{
    int i;

    if (table->num_available < MIN_PLAYERS) {
        broadcast(table, "not enough players to start game: %d", table->num_available);
        return;
    }

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->state == PLAYER_STATE_WAITING
                && table->players[i]->chips >= table->minimum_bet) {
                table->players[i]->state = PLAYER_STATE_ACTIVE;
                table->players[i]->hand_cards[0] = get_card(&table->deck);
                table->players[i]->hand_cards[1] = get_card(&table->deck);
                table->num_active++;
                send_msg(table->players[i]->user, "[PRE_FLOP] %s, %s",
                        card_to_string(table->players[i]->hand_cards[0]),
                        card_to_string(table->players[i]->hand_cards[1]));
                if (table->players[i]->user->type == USER_TYPE_WEBSOCKET) {
                    send_msg(table->players[i]->user, "{\"type\":\"cards\",\"data\":{\"cards\":[\"%s\",\"%s\"]}}",
                        card_to_string(table->players[i]->hand_cards[0]),
                        card_to_string(table->players[i]->hand_cards[1]));
                }
        }
    }

    // dealer move next
    table->dealer = next_player(table, table->dealer);

    // small blind
    table->small_blind = next_player(table, table->dealer);
    table->players[table->small_blind]->chips -= (table->minimum_bet >> 1);
    table->players[table->small_blind]->bet += (table->minimum_bet >> 1);
    broadcast(table, "player %d bet %d", table->small_blind, table->players[table->small_blind]->bet);

    // big blind
    table->big_blind = next_player(table, table->small_blind);
    table->players[table->big_blind]->chips -= table->minimum_bet;
    table->players[table->big_blind]->bet += table->minimum_bet;
    broadcast(table, "player %d raise to %d", table->big_blind, table->players[table->big_blind]->bet);

    // prepare table
    table->bet  = table->minimum_bet;
    table->turn = next_player(table, table->big_blind);
    table->action_mask = ACTION_FOLD | ACTION_ALL_IN | ACTION_CALL | ACTION_RAISE;
    table_reset_timeout(table);
    table->state = TABLE_STATE_PREFLOP;

    table_to_json(table, table->json_cache, sizeof(table->json_cache));
    report_table(table);
}

void table_flop(table_t *table)
{
    int i;

    table->community_cards[table->num_cards++] = get_card(&table->deck);
    table->community_cards[table->num_cards++] = get_card(&table->deck);
    table->community_cards[table->num_cards++] = get_card(&table->deck);

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if(table->players[i]->state ==  PLAYER_STATE_ACTIVE) {
            table->players[i]->talked = 0;
        }
    }

    broadcast(table, "[FLOP] %s, %s, %s",
            card_to_string(table->community_cards[0]),
            card_to_string(table->community_cards[1]),
            card_to_string(table->community_cards[2]));
    table->state = TABLE_STATE_FLOP;
    table->bet  = 0;
    table->minimum_bet = 100;
    table->minimum_raise = 100;
    table->turn = next_player(table, table->dealer);
    table->action_mask = ACTION_FOLD | ACTION_ALL_IN | ACTION_CHCEK | ACTION_BET;
    table_reset_timeout(table);
    table_to_json(table, table->json_cache, sizeof(table->json_cache));
    report_table(table);
}

void table_turn(table_t *table)
{
    int i;

    table->community_cards[table->num_cards++] = get_card(&table->deck);

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if(table->players[i]->state ==  PLAYER_STATE_ACTIVE) {
            table->players[i]->talked = 0;
        }
    }

    broadcast(table, "[TURN] %s %s %s %s",
            card_to_string(table->community_cards[0]),
            card_to_string(table->community_cards[1]),
            card_to_string(table->community_cards[2]),
            card_to_string(table->community_cards[3]));
    table->state = TABLE_STATE_TURN;
    table->bet  = 0;
    table->minimum_bet = 100;
    table->minimum_raise = 100;
    table->turn = next_player(table, table->dealer);
    table->action_mask = ACTION_FOLD | ACTION_ALL_IN | ACTION_CHCEK | ACTION_BET;
    table_reset_timeout(table);
    table_to_json(table, table->json_cache, sizeof(table->json_cache));
    report_table(table);
}

void table_river(table_t *table)
{
    int i;

    table->community_cards[table->num_cards++] = get_card(&table->deck);

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if(table->players[i]->state ==  PLAYER_STATE_ACTIVE) {
            table->players[i]->talked = 0;
        }
    }

    broadcast(table, "[RIVER] %s %s %s %s %s",
            card_to_string(table->community_cards[0]),
            card_to_string(table->community_cards[1]),
            card_to_string(table->community_cards[2]),
            card_to_string(table->community_cards[3]),
            card_to_string(table->community_cards[4]));
    table->state = TABLE_STATE_RIVER;
    table->bet  = 0;
    table->minimum_bet = 100;
    table->minimum_raise = 100;
    table->turn = next_player(table, table->dealer);
    table->action_mask = ACTION_FOLD | ACTION_ALL_IN | ACTION_CHCEK | ACTION_BET;
    table_reset_timeout(table);
    table_to_json(table, table->json_cache, sizeof(table->json_cache));
    report_table(table);
}

int cmp_by_rank(const void *a, const void *b)
{
    player_t **x = (player_t **)a, **y = (player_t **)b;
    return rank_cmp((*y)->rank, (*x)->rank);
}

void table_start(table_t *table)
{
    table_prepare(table);
    table_switch(table);
}

void table_finish(table_t *table)
{
    int i, j, k, l, possible_winners_count, chips;
    int pot_share_count[TABLE_MAX_PLAYERS];
    player_t *possible_winners[TABLE_MAX_PLAYERS];
    char buffer[1024];

    // collect main pot to side pots
    table->side_pots[table->pot_count++] = table->pot;
    table->pot = 0;

    // if only one player left
    if (table->num_active + table->num_all_in == 1) {
        for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
            if (table->players[i]->state == PLAYER_STATE_ACTIVE || table->players[i]->state == PLAYER_STATE_ALL_IN) {
                for (chips = 0, j = 0; j < table->pot_count; j++) {
                    chips += table->side_pots[j];
                    table->side_pots[j] = 0;
                }
                broadcast(table, "player %d wins %d", i, chips);
                table->players[i]->chips += chips;
                table_to_json(table, table->json_cache, sizeof(table->json_cache));
                report_table(table);
                table_prepare(table);
                return;
            }
        }
        err_quit("no winner");
    }
    
    // fill community cards
    while (table->num_cards < 5) {
        table->community_cards[table->num_cards++] = get_card(&table->deck);
    }

    // calc ranks & put players to possible winners
    for (i = 0, possible_winners_count = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->state == PLAYER_STATE_ACTIVE || table->players[i]->state == PLAYER_STATE_ALL_IN) {
            if (table->players[i]->state == PLAYER_STATE_ACTIVE) {
                table->players[i]->pot_offset = table->pot_count;
            }
            for (j = 0; j < 5; j++) {
                table->players[i]->hand_cards[j + 2] = table->community_cards[j];
            }
            table->players[i]->rank = calc_rank(table->players[i]->hand_cards);
            hand_to_string(table->players[i]->hand_cards, table->players[i]->rank, buffer, sizeof(buffer));
            broadcast(table, "player %d's cards is %s, %s. rank is [%s], hand is %s, score is %d",
                    i, card_to_string(table->players[i]->hand_cards[0]), card_to_string(table->players[i]->hand_cards[1]),
                    level_to_string(table->players[i]->rank.level), buffer, table->players[i]->rank.score);
            possible_winners[possible_winners_count++] = table->players[i];
        }
    }

    // sort by rank TODO equal
    qsort(possible_winners, possible_winners_count, sizeof(player_t*), cmp_by_rank);

    // distribute the pots to winners
    for (i = 0; i < possible_winners_count; i = j) {
        // we may have more than one winners with the same rank&score
        for (j = i + 1; j < possible_winners_count
                && possible_winners[j]->rank.level == possible_winners[i]->rank.level
                && possible_winners[j]->rank.score == possible_winners[i]->rank.score; j++);
        memset(pot_share_count, 0, sizeof pot_share_count);
        for (k = i; k < j; k++) {
            for (l = 0; l < possible_winners[k]->pot_offset; l++) {
                pot_share_count[l]++;
            }
        }

        for (k = i; k < j; k++) {
            for (chips = 0, l = 0; l < possible_winners[k]->pot_offset; l++) {
                chips               += table->side_pots[l] / pot_share_count[l];
                table->side_pots[l] -= table->side_pots[l] / pot_share_count[l];
            }
            possible_winners[k]->chips += chips;
            broadcast(table, "player %d wins %d", possible_winners[k] - table->players[0], chips);
        }
    }
    table_to_json(table, table->json_cache, sizeof(table->json_cache));
    report_table(table);
    table_prepare(table);
}

inline void table_init_timeout(table_t *table)
{
    table->ev_timeout = event_new(table->base, -1, 0, timeoutcb, table);
}

inline void table_reset_timeout(table_t *table)
{
    event_add(table->ev_timeout, &_timeout_minute);
}

inline void table_clear_timeout(table_t *table)
{
    event_del(table->ev_timeout);
}

int action_to_string(action_t mask, char *buffer, int size)
{
    int offset = 0;
    buffer[offset++] = '[';
    if (mask & ACTION_BET)    offset += snprintf(buffer + offset, size - offset, "bet|");
    if (mask & ACTION_RAISE)  offset += snprintf(buffer + offset, size - offset, "raise|");
    if (mask & ACTION_CALL)   offset += snprintf(buffer + offset, size - offset, "call|");
    if (mask & ACTION_CHCEK)  offset += snprintf(buffer + offset, size - offset, "check|");
    if (mask & ACTION_FOLD)   offset += snprintf(buffer + offset, size - offset, "fold|");
    if (mask & ACTION_ALL_IN) offset += snprintf(buffer + offset, size - offset, "all_in|");
    if (offset > 1) offset--;
    offset += snprintf(buffer + offset, size - offset, "]");
    return offset;
}

int action_to_json(action_t mask, char *buffer, int size)
{
    int offset = 0;
    buffer[offset++] = '[';
    if (mask & ACTION_BET)    offset += snprintf(buffer + offset, size - offset, "\"bet\",");
    if (mask & ACTION_RAISE)  offset += snprintf(buffer + offset, size - offset, "\"raise\",");
    if (mask & ACTION_CALL)   offset += snprintf(buffer + offset, size - offset, "\"call\",");
    if (mask & ACTION_CHCEK)  offset += snprintf(buffer + offset, size - offset, "\"check\",");
    if (mask & ACTION_FOLD)   offset += snprintf(buffer + offset, size - offset, "\"fold\",");
    if (mask & ACTION_ALL_IN) offset += snprintf(buffer + offset, size - offset, "\"all_in\",");
    if (offset > 1) offset--;
    offset += snprintf(buffer + offset, size - offset, "]");
    return offset;
}

void broadcast(table_t *table, const char *fmt, ...)
{
    int i;

    va_list ap;
    va_start(ap, fmt);
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->user) {
            send_msgv(table->players[i]->user, fmt, ap);
            if (table->players[i]->user->type == USER_TYPE_WEBSOCKET) {
                send_msg(table->players[i]->user, table->json_cache);
            }
        }
    }
    va_end(ap);
}

int player_join(table_t *table, user_t *user)
{
    int i;
    player_t *player;

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->user == NULL && table->players[i]->state == PLAYER_STATE_EMPTY) {
            player = table->players[i];
            player->user = user;
            user->table = table;
            user->index = i;
            user->state |= USER_STATE_TABLE;
            if (player->chips < table->minimum_bet && player->user->money > 10000 ) {
                player_buy_chips(player, 10000);
            }
            table_to_json(table, table->json_cache, sizeof(table->json_cache));
            broadcast(table, "player %d %s joined chips %d", i, user->name, player->chips);
            return TEXAS_RET_SUCCESS;
        }
    }
    return TEXAS_RET_MAX_PLAYER;
}

int player_quit(user_t *user)
{
    table_t *table = user->table;
    int index = user->index;
    player_t *player = table->players[index];

    if (table->state > TABLE_STATE_WAITING && player->state == PLAYER_STATE_ACTIVE && table->turn == user->index) {
        handle_action(table, table->turn, ACTION_FOLD, 0);
    }
    if (player->chips > 0) {
        user_add_money(player->user, player->chips);
        player->chips = 0;
    }
    player->user = NULL;
    user->table = NULL;
    user->index = -1;
    user->state &= ~USER_STATE_TABLE;
    table_to_json(table, table->json_cache, sizeof(table->json_cache));
    broadcast(table, "player %d quit", index);
    return TEXAS_RET_SUCCESS;
}

int next_player(table_t *table, int index)
{
    int i, next;

    for (i = 1; i < TABLE_MAX_PLAYERS; i++) {
        next = (index + i) % TABLE_MAX_PLAYERS;
        if (table->players[next]->state == PLAYER_STATE_ACTIVE) {
            return next;
        }
    }

    return -1;
}

int player_bet(table_t *table, int index, int bet)
{
    player_t *player = table->players[index];

    assert(player->bet == 0);
    assert(table->bet == 0);

    if (bet > player->chips) {
        return TEXAS_RET_LOW_POT;
    }
    if (bet < table->minimum_bet) {
        return TEXAS_RET_MIN_BET;
    }
    player->chips -= bet;
    player->bet += bet;
    table->bet = player->bet;
    table->minimum_raise = table->bet;
    player->talked = 1;

    return TEXAS_RET_SUCCESS;
}

int player_raise(table_t *table, int index, int raise)
{
    player_t *player = table->players[index];
    int diff = table->bet - player->bet;

    assert(player->chips >= diff + table->minimum_raise);

    if (raise < table->minimum_raise) {
        return TEXAS_RET_MIN_BET;
    }
    if (raise + diff > player->chips) {
        return TEXAS_RET_LOW_POT;
    }
    player->chips -= (raise + diff);
    player->bet += (raise + diff);
    table->bet = player->bet;
    table->minimum_raise = raise;
    player->talked = 1;

    return TEXAS_RET_SUCCESS;
}

int player_call(table_t *table, int index)
{
    player_t *player = table->players[index];
    int diff = table->bet - player->bet;

    assert(player->chips >= diff);

    player->chips -= diff;
    player->bet += diff;
    player->talked = 1;

    return TEXAS_RET_SUCCESS;
}

int player_all_in(table_t *table, int index)
{
    player_t *player = table->players[index];
    int diff;

    player->bet += player->chips;
    player->chips = 0;
    if (player->bet > table->bet) {
        diff = player->bet - table->bet;
        table->bet = player->bet;
        if (diff > table->minimum_raise) {
            table->minimum_raise = diff;
        }
    }
    player->talked = 1;
    table->num_all_in++;
    table->num_active--;
    player->state = PLAYER_STATE_ALL_IN;

    return TEXAS_RET_SUCCESS;
}

int player_fold(table_t *table, int index)
{
    player_t *player = table->players[index];

    table->num_active--;
    table->num_folded++;
    player->talked = 1;
    player->state = PLAYER_STATE_FOLDED;
    return TEXAS_RET_SUCCESS;
}

int player_check(table_t *table, int index)
{
    player_t *player = table->players[index];
    assert(table->bet == player->bet && player->talked == 0);

    player->talked = 1;
    return TEXAS_RET_SUCCESS;
}

int handle_action(table_t *table, int index, action_t action, int value)
{
    player_t *player = table->players[index];
    player_t *player_next;
    int next;

    // check action
    if (!(action & table->action_mask)) {
        send_msg(player->user, "invalid action");
        return TEXAS_RET_ACTION;
    }

    switch (action) {
    case ACTION_BET:
        if (player_bet(table, index, value) != TEXAS_RET_SUCCESS) {
            send_msg(player->user, "bet failed");
            return TEXAS_RET_FAILURE;
        }
        broadcast(table, "player %d bet %d", index, value);
        break;
    case ACTION_RAISE:
        if (player_raise(table, index, value) != TEXAS_RET_SUCCESS) {
            send_msg(player->user, "bet failed");
            return TEXAS_RET_FAILURE;
        }
        broadcast(table, "player %d raise %d", index, value);
        break;
    case ACTION_CALL:
        if (player_call(table, index) != TEXAS_RET_SUCCESS) {
            send_msg(player->user, "call failed");
            return TEXAS_RET_FAILURE;
        }
        broadcast(table, "player %d call", index);
        break;
    case ACTION_CHCEK:
        if (player_check(table, index) != TEXAS_RET_SUCCESS) {
            send_msg(player->user, "check failed");
            return TEXAS_RET_FAILURE;
        }
        broadcast(table, "player %d check", index);
        break;
    case ACTION_ALL_IN:
        if (player_all_in(table, index) != TEXAS_RET_SUCCESS) {
            send_msg(player->user, "all in failed");
            return TEXAS_RET_FAILURE;
        }
        broadcast(table, "player %d check", index);
        break;
    case ACTION_FOLD:
        if (player_fold(table, index) != TEXAS_RET_SUCCESS) {
            send_msg(player->user, "fold failed");
            return TEXAS_RET_FAILURE;
        }
        broadcast(table, "player %d fold", index);
        break;
    default:
        err_quit("default");
        break;
    }

    // generate available actions
    next = next_player(table, table->turn);

    if ((table->num_active == 1 && table->num_all_in == 0) || next < 0) {
        table_process(table);
        table_finish(table);
        return TEXAS_RET_SUCCESS;
    }

    player_next = table->players[next];
    assert(player_next);
    if (player_next->bet == table->bet && player_next->talked == 1) {
        table_process(table);
        table_switch(table);
        return TEXAS_RET_SUCCESS;
    }

    table->action_mask = 0;
    table->action_mask |= ACTION_FOLD;

    if (player_next->chips > 0) {
        table->action_mask |= ACTION_ALL_IN;
    }

    if (table->bet == player_next->bet && player_next->talked == 0) {
        table->action_mask |= ACTION_CHCEK;
    }

    if (table->bet == 0) {
        table->action_mask |= ACTION_BET;
    }

    if (player_next->bet < table->bet && player_next->chips + player_next->bet > table->bet) {
        table->action_mask |= ACTION_CALL;
    }

    if (table->bet > 0 && player_next->chips + player_next->bet >= table->bet + table->minimum_raise) {
        table->action_mask |= ACTION_RAISE;
    }

    table->turn = next;
    table_to_json(table, table->json_cache, sizeof(table->json_cache));
    report_table(table);
    if (player_next->user == NULL) {
        return handle_action(table, next, ACTION_FOLD, 0);
    }
    return TEXAS_RET_SUCCESS;
}

int cmp_by_bet(const void *a, const void *b)
{
    player_t **x = (player_t **)a, **y = (player_t **)b;
    return (((*x)->bet) - ((*y)->bet));
}

int table_process(table_t *table)
{
    player_t *all_in_players[TABLE_MAX_PLAYERS];
    int i, j, all_in_players_count = 0, bet, side_pot;

    // remove left player
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->user == NULL) {
            switch (table->players[i]->state) {
            case PLAYER_STATE_ALL_IN:
                // TODO all in && quit
                break;
            case PLAYER_STATE_ACTIVE:
                err_quit("active");
            case PLAYER_STATE_FOLDED:
                table->num_folded--;
            case PLAYER_STATE_EMPTY:
            case PLAYER_STATE_WAITING:
                table->players[i]->state = PLAYER_STATE_EMPTY;
                break;
            }
        }
    }

    // find all in players this round
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->state == PLAYER_STATE_ALL_IN && table->players[i]->bet > 0) {
            all_in_players[all_in_players_count++] = table->players[i];
        }
    }

    if (all_in_players_count > 0) {
        // sort
        qsort(all_in_players, all_in_players_count, sizeof(player_t *), cmp_by_bet);

        // calc side pots
        for (i = 0; i < all_in_players_count; i++) {
            bet = all_in_players[i]->bet;
            for (j = 0, side_pot = 0; j < TABLE_MAX_PLAYERS; j++) {
                if (table->players[j]->bet > 0) {
                    if (table->players[j]->bet < bet) {
                        side_pot += table->players[j]->bet;
                        table->players[j]->bet = 0;
                    } else {
                        side_pot += bet;
                        table->players[j]->bet -= bet;
                    }
                }
            }
            side_pot += table->pot;
            table->pot = 0;
            table->side_pots[table->pot_count] = side_pot;
            all_in_players[i]->pot_offset = ++(table->pot_count);
        }
    }

    // collect bets to table's main pot
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->bet > 0) {
            table->pot += table->players[i]->bet;
            table->players[i]->bet = 0;
        }
    }
    return TEXAS_RET_SUCCESS;
}

int table_switch(table_t *table)
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
        table_finish(table);
        break;
    default:
        break;
    }
    return TEXAS_RET_SUCCESS;
}

int player_buy_chips(player_t *player, int chips)
{
    assert(player->user);
    if (player->user->money < chips) {
        return -1;
    }
    player->user->money -= chips;
    player->chips += chips;
    if (user_save(player->user) < 0) {
        return -1;
    }
    return 0;
}

void report_table(table_t *table)
{
    char buffer[1024], mask_buffer[1024];

    action_to_string(table->action_mask, mask_buffer, sizeof(mask_buffer));
    snprintf(buffer, sizeof(buffer), "turn player %d bet %d pot %d mask %s",
            table->turn, table->bet, table->pot, mask_buffer);
    broadcast(table, buffer);
}

int table_chat(table_t *table, int index, const char *msg)
{
    broadcast(table, "player %d says: %s", index, msg);
    return 0;
}

int table_to_json(table_t *table, char *buffer, int size)
{
    int i, offset = 0;

    offset += snprintf(buffer + offset, size - offset, "{\"type\":\"table\",\"data\":{\"name\":\"%s\",\"turn\":%d,\"dealer\":%d,\"state\":%d,\"bet\":%d,\"min\":%d,\"pot\":%d,\"actions\":",
            table->name, table->turn, table->dealer, table->state, table->bet, table->minimum_raise, table->pot);
    offset += action_to_json(table->action_mask, buffer + offset, size - offset);

    if (table->num_cards > 0) {
        offset += snprintf(buffer + offset, size - offset, ",\"cards\":[");
        for (i = 0; i < table->num_cards; i++) {
            offset += snprintf(buffer + offset, size - offset, "\"%s\",", card_to_string(table->community_cards[i]));
        }
        buffer[offset - 1] = ']';
    }

    if (table->pot_count > 0) {
        offset += snprintf(buffer + offset, size - offset, ",\"side_pots\":[");
        for (i = 0; i < table->pot_count; i++) {
            offset += snprintf(buffer + offset, size - offset, "%d,", table->side_pots[i]);
        }
        buffer[offset - 1] = ']';
    }

    offset += snprintf(buffer + offset, size - offset, ",\"players\":[");
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->user) {
            offset += snprintf(buffer + offset, size - offset, "{\"player\":%d,\"name\":\"%s\",\"state\":%d,\"chips\":%d,\"bet\":%d,\"pot_offset\":%d},",
                    i, table->players[i]->user->name, table->players[i]->state, table->players[i]->chips, table->players[i]->bet, table->players[i]->pot_offset);
        }
    }
    if (buffer[offset - 1] == ',') {
        offset--;
    }

    offset += snprintf(buffer + offset, size - offset, "]}}");
    return offset;
}
