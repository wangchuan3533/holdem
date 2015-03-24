#include "hand.h"
#include <stdio.h>
#define CARD_BASE 13
#define HAND_NUM 5
#define COLOR_NUM 4
hand_rank_t calc_rank(hand_t hand)
{
    int i, j, numbers[CARD_BASE] = {0}, colors[COLOR_NUM] = {0};
    int flush = 0, straight = 0, four = 0, three = 0, two = 0;
    hand_rank_t rank;

    // prepare
    for (i = 0; i < HAND_NUM; i++) {
        numbers[hand[i] % CARD_BASE]++;
        colors[hand[i] / CARD_BASE]++;
    }

    for (i = 0; i < COLOR_NUM; i++) {
        if (colors[i] == HAND_NUM) {
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
        for (i = 0; i < CARD_BASE - HAND_NUM; i++) {
            for (j = 0; j < HAND_NUM; j++) {
                if (!numbers[i + j]) {
                    break;
                }
            }
            if (j == HAND_NUM) {
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
