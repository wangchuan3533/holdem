#include "hand.h"
#include <stdio.h>

hand_rank_t _calc_rank(hand_t hand);
unsigned int bit_count(unsigned int i);
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

inline const char *level_to_string(hand_level_t level)
{
    return level_string[level];
}

hand_rank_t calc_rank(hand_t hand)
{
    card_t cards[CARD_NUM];
    unsigned int i, j, k;
    hand_rank_t max_rank, rank;

    max_rank.level = 0;
    max_rank.score = 0;
    for (i = 0; i < (1u << HAND_NUM); i++) {
        if (bit_count(i) != CARD_NUM) {
            continue;
        }
        for (j = 0, k = 0; j < HAND_NUM; j++) {
            if (i & (1u << j)) {
                cards[k++] = hand[j];
            }
        }
        
        rank = _calc_rank(cards);
        rank.mask = i;
        if (rank_cmp(rank, max_rank) > 0) {
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
                rank.score *= CARD_BASE;
                rank.score += j;
            }
        }
    }

    // calc levels
    if (flush && straight) {
        rank.level = numbers[CARD_BASE - 1] ? ROYAL_FLUSH : STRAIGHT_FLUSH;
        return rank;
    }

    if (statistics[4]) {
        rank.level = FOUR_OF_A_KIND;
        return rank;
    }
    
    if (statistics[3] && statistics[2]) {
        rank.level = FULL_HORSE;
        return rank;
    }

    if (flush) {
        rank.level = FLUSH;
        return rank;
    }

    if (straight) {
        rank.level = STRAIGHT;
        return rank;
    }
    
    if (statistics[3]) {
        rank.level = THREE_OF_A_KIND;
        return rank;
    }

    if (statistics[2] >= 2) {
        rank.level = TWO_PAIR;
        return rank;
    }

    if (statistics[2]) {
        rank.level = ONE_PAIR;
        return rank;
    }
    rank.level = HIGH_CARD;
    return rank;
}

// see hamming weight(http://en.wikipedia.org/wiki/Hamming_weight)
inline unsigned int bit_count(unsigned int i)
{
     i = i - ((i >> 1) & 0x55555555);
     i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
     return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

inline int rank_cmp(hand_rank_t r1, hand_rank_t r2)
{
    if (r1.level > r2.level) {
        return 1;
    }
    if (r1.level < r2.level) {
        return -1;
    }
    if (r1.score > r2.score) {
        return 1;
    }
    if (r1.score < r2.score) {
        return -1;
    }
    return 0;
}

int hand_to_string(card_t *cards, hand_rank_t rank, char *buffer, int size)
{
    int i, offset = 0;

    for (i = 0; i < HAND_NUM; i++) {
        if (rank.mask & (1u << i)) {
            offset += snprintf(buffer + offset, size - offset, "%s, ", card_to_string(cards[i]));
        }
    }
    buffer[--offset] = '\0';
    return offset;
}
