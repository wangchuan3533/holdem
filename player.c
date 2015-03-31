#include <string.h>
#include <stdio.h>
#include "player.h"
player_t *g_current_player;
player_t *g_players = NULL;
int g_num_players = 0;

player_t *player_create()
{
    player_t *player;

    if (g_num_players >= MAX_PLAYERS) {
        return NULL;
    }
    player = (player_t *)malloc(sizeof(player_t));
    if (player == NULL) {
        return NULL;
    }
    memset(player, 0, sizeof(player_t));
    g_num_players++;
    return player;
}

void player_destroy(player_t *player)
{
    if (player) {
        g_num_players--;
        free(player);
    }
}

int player_to_json(player_t *player, char *buffer, int size)
{
    return snprintf(buffer, size,
            "{\"name\":\"%s\",\"pot\":%d,\"bid\":%d,\"state\":%d}",
            player->name, player->pot, player->bid, player->state);
}
