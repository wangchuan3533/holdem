#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include "card.h"
#include "hand.h"
#define TEST_CASES 7

int test_rank()
{
    int i;
    card_t cases[TEST_CASES][7] = {
        {0, 1, 8, 9, 10, 11, 12},
        {4, 5, 6, 7, 8, 10, 11},
        {10, 23, 36, 49, 1, 2, 3},
        {9, 22, 35, 12, 25, 2, 3},
        {3, 12, 2, 6, 8, 22, 23},
        {2, 15, 28, 9, 12, 8, 13},
        {0, 1, 2, 3, 25, 21, 13},
    };
    hand_rank_t ranks[TEST_CASES];


    for (i = 0; i < TEST_CASES; i++) {
        ranks[i] = calc_rank(cases[i]);
    }

    assert(GET_RANK(ranks[0].score) == STRAIGHT_FLUSH  && (ranks[0].score & 0xfffff) == 0xcba98 && ranks[0].mask == 0b1111100);
    assert(GET_RANK(ranks[1].score) == STRAIGHT_FLUSH  && (ranks[1].score & 0xfffff) == 0x87654 && ranks[1].mask == 0b0011111);
    assert(GET_RANK(ranks[2].score) == FOUR_OF_A_KIND  && (ranks[2].score & 0xfffff) == 0x000a3 && ranks[2].mask == 0b1001111);
    assert(GET_RANK(ranks[3].score) == FULL_HORSE      && (ranks[3].score & 0xfffff) == 0x0009c && ranks[3].mask == 0b0011111);
    assert(GET_RANK(ranks[4].score) == FLUSH           && (ranks[4].score & 0xfffff) == 0xc8632 && ranks[4].mask == 0b0011111);
    assert(GET_RANK(ranks[5].score) == THREE_OF_A_KIND && (ranks[5].score & 0xfffff) == 0x002c9 && ranks[5].mask == 0b0011111);
    assert(GET_RANK(ranks[6].score) == STRAIGHT        && (ranks[6].score & 0xfffff) == 0x03210 && ranks[6].mask == 0b0011111);
    printf("test_hand passed\n");

    return 0;
}

int test_card()
{
    uint64_t i, mask;
    card_t card;
    deck_t deck;
    

    init_deck(&deck);
    for (i = 0, mask = 0; i < 52; i++) {
        card = get_card(&deck);
        mask |= (1UL << card);
    }

    assert(mask == 0xfffffffffffffUL);
    printf("test_card passed\n");
    return 0;
}
int main()
{
    test_card();
    test_rank();
    return 0;
}
