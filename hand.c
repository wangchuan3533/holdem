#include "hand.h"
#include <stdio.h>

hand_rank_t _calc_rank(hand_t hand);
unsigned int bit_count(unsigned int i);

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
        if (rank_cmp(rank, max_rank) > 0) {
            max_rank = rank;
        }
    }
    return max_rank;
}

hand_rank_t _calc_rank(hand_t hand)
{
    int i, j, numbers[CARD_BASE] = {0}, colors[COLOR_NUM] = {0};
    int flush = 0, straight = 0, four = 0, three = 0, two = 0;
    hand_rank_t rank;

    // prepare
    for (i = 0; i < CARD_NUM; i++) {
        numbers[hand[i] % CARD_BASE]++;
        colors[hand[i] / CARD_BASE]++;
    }

    for (i = 0; i < COLOR_NUM; i++) {
        if (colors[i] == CARD_NUM) {
            flush = 1;
        }
    }

    for (i = 0; i < CARD_BASE; i++) {
        switch (numbers[i]) {
        case 4:
            four++;
            break;
        case 3:
            three++;
            break;
        case 2:
            two++;
            break;
        }
    }

    if (!(two || three || four)) {
        for (i = 0; i < CARD_BASE - CARD_NUM; i++) {
            for (j = 0; j < CARD_NUM; j++) {
                if (!numbers[i + j]) {
                    break;
                }
            }
            if (j == CARD_NUM) {
                straight = 1;
            }
        }
    }

    // calculate score
    rank.score = 0;
    for (i = CARD_BASE - 1; i >= 0; i--) {
        if (numbers[i]) {
            rank.score *= CARD_BASE;
            rank.score += numbers[i];
        }
    }

    // work
    if (flush && straight) {
        rank.level = numbers[CARD_BASE - 1] ? ROYAL_FLUSH : STRAIGHT_FLUSH;
        return rank;
    }

    if (four) {
        rank.level = FOUR_OF_A_KIND;
        return rank;
    }
    
    if (three && two) {
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
    
    if (three) {
        rank.level = THREE_OF_A_KIND;
        return rank;
    }

    if (two >= 2) {
        rank.level = TWO_PAIR;
        return rank;
    }

    if (two) {
        rank.level = ONE_PAIR;
        return rank;
    }
    rank.level = HIGH_CARD;
    return rank;
}

unsigned int bit_count(unsigned int i)
{
     i = i - ((i >> 1) & 0x55555555);
     i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
     return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

int rank_cmp(hand_rank_t r1, hand_rank_t r2)
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

