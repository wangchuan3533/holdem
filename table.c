#include "texas_holdem.h"
#include "table.h"

table_t *g_tables = NULL;
int g_num_tables = 0;
static struct timeval _timeout_minute = {60, 0}, _timeout_second = {1, 0};

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
        if (table->players) {
            free(table->players);
        }
        free(table);
        g_num_tables--;
    }
}

void table_reset(table_t *table)
{
    int i;

    table->pot           = 0;
    table->bet           = 0;
    table->small_blind   = 50;
    table->big_blind     = 100;
    table->minimum_bet   = 100;
    table->minimum_raise = 100;
    table->raise_count   = 0;
    table->num_players   = 0;
    table->num_active    = 0;
    table->num_all_in    = 0;
    table->num_folded    = 0;
    init_deck(&(table->deck));
    table_clear_timeout(table);

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        table->players[i]->bet        = 0;
        table->players[i]->talked     = 0;
        table->players[i]->rank.level = 0;
        table->players[i]->rank.score = 0;
        table->players[i]->left       = 0;
        table->players[i]->pot_index  = -1;

        if (table->players[i]->user) {
            table->players[i]->state = PLAYER_STATE_WAITING;
            table->num_players++;
        } else {
            table->players[i]->state = PLAYER_STATE_EMPTY;
        }
    }

    table->state  = TABLE_STATE_WAITING;
}

void table_pre_flop(table_t *table)
{
    int i, playing;

    for (i = 0, playing = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->state == PLAYER_STATE_WAITING
                && table->players[i]->chips >= table->minimum_bet) {
            playing++;
        }
    }
    if (playing < MIN_PLAYERS) {
        return;
    }
    for (i = 0, playing = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->state == PLAYER_STATE_WAITING
                && table->players[i]->chips >= table->minimum_bet) {
                table->players[i]->state = PLAYER_STATE_ACTIVE;
                table->players[i]->hand_cards[0] = get_card(&table->deck);
                table->players[i]->hand_cards[1] = get_card(&table->deck);
                table->num_active++;
                send_msg(table->players[i]->user, "[PRE_FLOP] %s, %s",
                        card_to_string(table->players[i]->hand_cards[0]),
                        card_to_string(table->players[i]->hand_cards[1]));
        }
    }

    // dealer move next
    table->dealer = next_player(table, table->dealer);

    // small blind
    table->small_blind = next_player(table, table->dealer);
    table->players[table->small_blind]->chips -= (table->minimum_bet >> 1);
    table->players[table->small_blind]->bet += (table->minimum_bet >> 1);
    broadcast(table, "%s blind %d", table->players[table->small_blind]->user->name,
            table->players[table->small_blind]->bet);

    // big blind
    table->big_blind = next_player(table, table->small_blind);
    table->players[table->big_blind]->chips -= table->minimum_bet;
    table->players[table->big_blind]->bet += table->minimum_bet;
    broadcast(table, "%s blind %d", table->players[table->big_blind]->user->name, table->players[table->big_blind]->bet);

    // prepare table
    table->bet  = table->minimum_bet;
    table->turn = next_player(table, table->big_blind);
    table->action_mask = ACTION_FOLD | ACTION_ALL_IN | ACTION_CALL | ACTION_RAISE;
    table_reset_timeout(table);
    table->state = TABLE_STATE_PREFLOP;

    report(table);
}

void table_flop(table_t *table)
{
    int i;

    table->community_cards[0] = get_card(&table->deck);
    table->community_cards[1] = get_card(&table->deck);
    table->community_cards[2] = get_card(&table->deck);

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
    table->turn = next_player(table, table->dealer);
    table->action_mask = ACTION_FOLD | ACTION_ALL_IN | ACTION_CHCEK | ACTION_BET;
    table_reset_timeout(table);
    report(table);
}

void table_turn(table_t *table)
{
    int i;

    table->community_cards[3] = get_card(&table->deck);

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if(table->players[i]->state ==  PLAYER_STATE_ACTIVE) {
            table->players[i]->talked = 0;
        }
    }

    broadcast(table, "[TURN] %s", card_to_string(table->community_cards[3]));
    table->state = TABLE_STATE_TURN;
    table->bet  = 0;
    table->turn = next_player(table, table->dealer);
    table->action_mask = ACTION_FOLD | ACTION_ALL_IN | ACTION_CHCEK | ACTION_BET;
    table_reset_timeout(table);
    report(table);
}

void table_river(table_t *table)
{
    int i;

    table->community_cards[4] = get_card(&table->deck);

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if(table->players[i]->state ==  PLAYER_STATE_ACTIVE) {
            table->players[i]->talked = 0;
        }
    }

    broadcast(table, "[RIVER] %s", card_to_string(table->community_cards[4]));
    table->state = TABLE_STATE_RIVER;
    table->bet  = 0;
    table->turn = next_player(table, table->dealer);
    table->action_mask = ACTION_FOLD | ACTION_ALL_IN | ACTION_CHCEK | ACTION_BET;
    table_reset_timeout(table);
    report(table);
}

int cmp_by_rank(const void *a, const void *b)
{
    player_t **x = (player_t **)a, **y = (player_t **)b;
    return rank_cmp((*x)->rank, (*y)->rank);
}

void table_showdown(table_t *table)
{
    int i, j, possible_winners_count, chips;
    player_t *possible_winners[TABLE_MAX_PLAYERS];

    switch (table->state) {
    case TABLE_STATE_PREFLOP:
        table->community_cards[0] = get_card(&table->deck);
        table->community_cards[1] = get_card(&table->deck);
        table->community_cards[2] = get_card(&table->deck);
    case TABLE_STATE_FLOP:
        table->community_cards[3] = get_card(&table->deck);
    case TABLE_STATE_TURN:
        table->community_cards[4] = get_card(&table->deck);
    case TABLE_STATE_RIVER:
        break;
    default:
        err_quit("default");
    }

    broadcast(table, "[SHOWDOWN] community cards is %s, %s, %s, %s, %s",
            card_to_string(table->community_cards[0]),
            card_to_string(table->community_cards[1]),
            card_to_string(table->community_cards[2]),
            card_to_string(table->community_cards[3]),
            card_to_string(table->community_cards[4]));
    for (i = 0, possible_winners_count = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->state == PLAYER_STATE_ACTIVE || table->players[i]->state == PLAYER_STATE_ALL_IN) {
            for (j = 0; j < 5; j++) {
                table->players[i]->hand_cards[j + 2] = table->community_cards[j];
            }
            table->players[i]->rank = calc_rank(table->players[i]->hand_cards);
            hand_to_string(table->players[i]->hand_cards, table->players[i]->rank, table->buffer, sizeof(table->buffer));
            broadcast(table, "player %s's cards is %s, %s. rank is [%s], hand is %s, score is %d",
                    table->players[i]->user->name, card_to_string(table->players[i]->hand_cards[0]),
                    card_to_string(table->players[i]->hand_cards[1]), level_to_string(table->players[i]->rank.level),
                    table->buffer, table->players[i]->rank.score);
            possible_winners[possible_winners_count++] = table->players[i];
        }
    }

    // sort by rank TODO equal
    qsort(possible_winners, sizeof(player_t*), possible_winners_count, cmp_by_rank);
    for (i = 0; i < possible_winners_count; i++) {
        if (possible_winners[i]->state == PLAYER_STATE_ALL_IN) {
            for (chips = 0, j = possible_winners[i]->pot_index; j >= 0; j--) {
                chips += table->pots[j].chips;
                table->pots[j].chips = 0;
            }
            broadcast(table, "player %s wins %d", possible_winners[i]->user->name, chips);
        } else {
            for (chips = 0, j = table->pot_offset - 1; j >= 0; j--) {
                chips += table->pots[j].chips;
                table->pots[j].chips = 0;
            }
            chips += table->pot;
            table->pot = 0;
            broadcast(table, "player %s wins %d", possible_winners[i]->user->name, chips);
        }
        possible_winners[i]->chips += chips;
    }

    table_reset(table);
    handle_table(table);
}

int table_check_winner(table_t *table)
{
    int i;

    assert(table->num_active == 1 && table->num_all_in == 0);

    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i] && (table->players[i]->state == PLAYER_STATE_ACTIVE)) {
            broadcast(table, "[WINNER] %s [POT] %d", table->players[i]->user->name, table->pot);
            table->players[i]->chips += table->pot;
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
    event_add(table->ev_timeout, &_timeout_minute);
}

inline void table_clear_timeout(table_t *table)
{
    event_del(table->ev_timeout);
}

int action_to_string(action_t mask, char *buffer, int size)
{
    return snprintf(buffer, size, "[%s|%s|%s|%s|%s|%s]",
            mask & ACTION_BET    ? "bet"    : "",
            mask & ACTION_RAISE  ? "raise"  : "",
            mask & ACTION_CALL   ? "call"   : "",
            mask & ACTION_CHCEK  ? "check"  : "",
            mask & ACTION_FOLD   ? "fold"   : "",
            mask & ACTION_ALL_IN ? "all_in" : "");
}

void broadcast(table_t *table, const char *fmt, ...)
{
    int i;

    va_list ap;
    va_start(ap, fmt);
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->user) {
            send_msgv(table->players[i]->user, fmt, ap);
        }
    }
    va_end(ap);
}

int player_join(table_t *table, user_t *user)
{
    int i;

    if (table->num_players >= TABLE_MAX_PLAYERS) {
        return TEXAS_RET_MAX_PLAYER;
    }
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->user == NULL) {
            table->players[i]->user = user;
            user->table = table;
            user->state |= USER_STATE_TABLE;
            user->index = i;
            table->num_players++;
            broadcast(table, "%s joined", user->name);
            return TEXAS_RET_SUCCESS;
        }
    }
    err_quit("join");
    return TEXAS_RET_FAILURE;
}

int player_quit(user_t *user)
{
    if (user->table->players[user->index]->user == user) {
        user->table->num_players--;
        user->table->players[user->index]->user = NULL;
        user->table = NULL;
        broadcast(user->table, "%s quit", user->name);
        user->state &= ~USER_STATE_TABLE;
        return TEXAS_RET_SUCCESS;
    }
    // fatal
    err_quit("quit");
    return TEXAS_RET_FAILURE;
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
    table->raise_count++;
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
            table->raise_count++;
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

    player->bet = 0;
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

    if (player->state == PLAYER_STATE_ALL_IN) {
        send_msg(player->user, "you already all in");
        return TEXAS_RET_ALL_IN;
    }

    switch (action) {
    case ACTION_BET:
        if (player_bet(table, index, value) != TEXAS_RET_SUCCESS) {
            send_msg(player->user, "bet failed");
            return TEXAS_RET_FAILURE;
        }
        broadcast(table, "%s bet %d", player->user->name, value);
        break;
    case ACTION_RAISE:
        if (player_raise(table, index, value) != TEXAS_RET_SUCCESS) {
            send_msg(player->user, "raise failed");
            return TEXAS_RET_FAILURE;
        }
        broadcast(table, "%s raise %d", player->user->name, value);
        break;
    case ACTION_CALL:
        if (player_call(table, index) != TEXAS_RET_SUCCESS) {
            send_msg(player->user, "call failed");
            return TEXAS_RET_FAILURE;
        }
        broadcast(table, "%s call", player->user->name);
        break;
    case ACTION_CHCEK:
        if (player_check(table, index) != TEXAS_RET_SUCCESS) {
            send_msg(player->user, "check failed");
            return TEXAS_RET_FAILURE;
        }
        broadcast(table, "%s check", player->user->name);
        break;
    case ACTION_ALL_IN:
        if (player_all_in(table, index) != TEXAS_RET_SUCCESS) {
            send_msg(player->user, "check failed");
            return TEXAS_RET_FAILURE;
        }
        broadcast(table, "%s all in", player->user->name);
        break;
    case ACTION_FOLD:
        if (player_fold(table, index) != TEXAS_RET_SUCCESS) {
            send_msg(player->user, "check failed");
            return TEXAS_RET_FAILURE;
        }
        broadcast(table, "%s fold", player->user->name);
        break;
    default:
        err_quit("default");
        break;
    }

    // generate available actions
    next = next_player(table, table->turn);

    if (next < 0 && table->num_active == 1) {
        if (table->num_all_in > 0) {
            table_showdown(table);
            return TEXAS_RET_SUCCESS;
        }
        handle_pot(table);
        assert(table_check_winner(table) == 0);
        handle_table(table);
        return TEXAS_RET_SUCCESS;
    }

    player_next = table->players[next];
    assert(player_next);
    if (player_next->bet == table->bet && player_next->talked == 1) {
        handle_pot(table);
        handle_table(table);
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
    table_reset_timeout(table);
    report(table);
    return TEXAS_RET_SUCCESS;
}

int cmp_by_bet(const void *a, const void *b)
{
    player_t **x = (player_t **)a, **y = (player_t **)b;
    return (((*x)->bet) - ((*y)->bet));
}

int handle_pot(table_t *table)
{
    player_t *all_in_players[TABLE_MAX_PLAYERS];
    int i, j, all_in_players_count, bet, side_pot;

    // find all in players this round
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->state != PLAYER_STATE_EMPTY) {
            if (table->players[i]->state == PLAYER_STATE_ALL_IN && table->players[i]->bet > 0) {
                all_in_players[all_in_players_count++] = table->players[i];
            }
        }
    }

    if (all_in_players_count > 0) {
        // sort
        qsort(all_in_players, sizeof(player_t *), all_in_players_count, cmp_by_bet);

        // calc side pots
        for (i = 0; i < all_in_players_count; i++) {
            bet = all_in_players[i]->bet;
            for (j = 0; j < TABLE_MAX_PLAYERS; j++) {
                if (table->players[j]->state != PLAYER_STATE_WAITING && table->players[j]->bet > 0) {
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
            table->pots[table->pot_offset].chips = side_pot;
            table->pots[table->pot_offset].index = all_in_players[i] - table->players[0];
            all_in_players[i]->pot_index = table->pot_offset++;
        }
    }

    // collect bets to table's main pot
    for (i = 0; i < TABLE_MAX_PLAYERS; i++) {
        if (table->players[i]->state != PLAYER_STATE_WAITING && table->players[i]->bet > 0) {
            table->pot += table->players[i]->bet;
            table->players[i]->bet = 0;
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

