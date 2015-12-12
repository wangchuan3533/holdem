#include "hand.h"
#include <stdio.h>

hand_rank_t _calc_rank(hand_t hand);
static const char *level_string[] = {
    "HIGH_CARD",
    "ONE_PAIR",
    "TWO_PAIR",
    "THREE_OF_A_KIND",
    "STRAIGHT",
    "FLUSH",
    "FULL_HORSE",
    "FOUR_OF_A_KIND",
    "STRAIGHT_FLUSH",
    "ROYAL_FLUSH",
};

static unsigned int masks[] = {
    0x1f, 0x2f, 0x37, 0x3b, 0x3d, 0x3e, 0x4f,
    0x57, 0x5b, 0x5d, 0x5e, 0x67, 0x6b, 0x6d,
    0x6e, 0x73, 0x75, 0x76, 0x79, 0x7a, 0x7c,
};
#define NUM_MASKS (sizeof(masks) / sizeof(unsigned int))


inline const char *level_to_string(hand_level_t level)
{
    return level_string[level];
}

hand_rank_t calc_rank(hand_t hand)
{
    card_t cards[CARD_NUM];
    unsigned int i, j, k;
    hand_rank_t max_rank, rank;

    max_rank.score = 0;
    for (i = 0; i < NUM_MASKS; i++) {
        for (j = 0, k = 0; j < HAND_NUM; j++) {
            if (masks[i] & (1u << j)) {
                cards[k++] = hand[j];
            }
        }
        
        rank = _calc_rank(cards);
        rank.mask = masks[i];
        if (rank.score > max_rank.score) {
            max_rank = rank;
        }
    }
    return max_rank;
}

hand_rank_t _calc_rank(hand_t hand)
{
    int i, j, numbers[CARD_BASE] = {0}, suits[4] = {0}, statistics[CARD_NUM] = {0};
    int flush = 0, straight = 0;
    hand_rank_t rank;

    // prepare
    for (i = 0; i < CARD_NUM; i++) {
        numbers[hand[i] % CARD_BASE]++;
        suits[hand[i] / CARD_BASE]++;
    }

    // statistics
    for (i = 0; i < CARD_BASE; i++) {
        statistics[numbers[i]]++;
    }

    // check flush
    for (i = 0; i < 4; i++) {
        if (suits[i] == CARD_NUM) {
            flush = 1;
        }
    }

    // check straight
    if (!(statistics[2] || statistics[3] || statistics[4])) {
        for (i = 0; numbers[i] == 0 && i < CARD_BASE; i++);
        for (j = 0; j < 5 && i + j < CARD_BASE; j++) {
            if (!numbers[i + j]) {
                break;
            }
        }
        if (j == 5) {
            straight = 1;
        } else if (i == 4 && j == 5 && numbers[CARD_BASE - 1] > 0) {
            straight = 1;
            numbers[CARD_BASE - 1] = 0;
        }
    }

    // calculate score
    rank.score = 0;
    for (i = 4; i > 0; i--) {
        if (statistics[i] == 0) {
            continue;
        }
        for (j = CARD_BASE - 1; j >= 0; j--) {
            if (numbers[j] == i) {
                rank.score <<= 4;
                rank.score |= j;
            }
        }
    }

    // calc levels
    if (flush && straight) {
        SET_RANK(rank.score, STRAIGHT_FLUSH);
    } else if (statistics[4]) {
        SET_RANK(rank.score, FOUR_OF_A_KIND);
    } else if (statistics[3] && statistics[2]) {
        SET_RANK(rank.score, FULL_HORSE);
    } else if (flush) {
        SET_RANK(rank.score, FLUSH);
    } else if (straight) {
        SET_RANK(rank.score, STRAIGHT);
    } else if (statistics[3]) {
        SET_RANK(rank.score, THREE_OF_A_KIND);
    } else if (statistics[2] >= 2) {
        SET_RANK(rank.score, TWO_PAIR);
    } else if (statistics[2]) {
        SET_RANK(rank.score, ONE_PAIR);
    } else {
        SET_RANK(rank.score, HIGH_CARD);
    }
    return rank;
}

// see hamming weight(http://en.wikipedia.org/wiki/Hamming_weight)
inline unsigned int bit_count(unsigned int i)
{
     i = i - ((i >> 1) & 0x55555555);
     i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
     return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

int hand_to_string(card_t *cards, hand_rank_t rank, char *buffer, int size)
{
    int i, offset = 0;

    for (i = 0; i < HAND_NUM; i++) {
        if (rank.mask & (1u << i)) {
            offset += snprintf(buffer + offset, size - offset, "%s,", card_to_string(cards[i]));
        }
    }
    buffer[--offset] = '\0';
    return offset;
}
