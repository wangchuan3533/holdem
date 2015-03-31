#include <string.h>
#include <stdio.h>
#include "player.h"
static player_t _players[MAX_PLAYERS];
static int _player_offset = 0;
player_t *g_current_player;

player_t *available_player()
{
    // todo check
    int i;
    for (i = 0; i < MAX_PLAYERS && _players[(_player_offset + i) % MAX_PLAYERS].state != PLAYER_STATE_EMPTY; i++);
    if (i == MAX_PLAYERS) {
        return NULL;
    }
    _player_offset = (_player_offset + i) % MAX_PLAYERS;
    return _players + _player_offset;
}

int player_to_json(player_t *player, char *buffer, int size)
{
    return snprintf(buffer, size,
            "{\"name\":\"%s\",\"pot\":%d,\"bid\":%d,\"state\":%d}",
            player->name, player->pot, player->bid, player->state);
}

int list_players()
{
    int i;
    for (i = 0; i < _player_offset; i++) {
        if (_players[i].state == PLAYER_STATE_WAITING
                || _players[i].state == PLAYER_STATE_GAME
                || _players[i].state == PLAYER_STATE_FOLDED) {
            send_msg(g_current_player, "%s%c", _players[i].name, i == (_player_offset - 1) ? ' ' : '\n');
        }
    }
}
