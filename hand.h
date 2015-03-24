#ifndef _HAND_H
#define _HAND_H
#include "card.h"
typedef card_t *hand_t;

typedef enum hand_level_s {
    HIGH_CARD,
    ONE_PAIR,
    TWO_PAIR,
    THREE_OF_A_KIND,
    STRAIGHT,
    FLUSH,
    FULL_HORSE,
    FOUR_OF_A_KIND,
    STRAIGHT_FLUSH,
    ROYAL_FLUSH,
} hand_level_t;

typedef struct  hand_rank_s {
    hand_level_t level;
    int score;
} hand_rank_t;

hand_rank_t calc_rank(hand_t hand);
#endif
