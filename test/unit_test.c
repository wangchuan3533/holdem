#include <stdio.h>
#include <assert.h>
#include "card.h"
#include "hand.h"
#define TEST_CASES 6

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
    };
    hand_rank_t ranks[TEST_CASES];


    for (i = 0; i < TEST_CASES; i++) {
        ranks[i] = calc_rank(cases[i]);
    }

    assert(GET_RANK(ranks[0].score) == STRAIGHT_FLUSH && ranks[0].score == 0x8cba98);
    assert(GET_RANK(ranks[1].score) == STRAIGHT_FLUSH && ranks[1].score == 0x887654);
    assert(GET_RANK(ranks[2].score) == FOUR_OF_A_KIND && ranks[2].score == 0x7000a3);
    assert(GET_RANK(ranks[3].score) == FULL_HORSE && ranks[3].score == 0x60009c);
    assert(GET_RANK(ranks[4].score) == FLUSH && ranks[4].score == 0x5c8632);
    assert(GET_RANK(ranks[5].score) == THREE_OF_A_KIND && ranks[5].score == 0x3002c9);

    return 0;
}

int test_card()
{
    deck_t deck;
    int i;

    init_deck(&deck);
    for (i = 0; i < 52; i++) {
        printf("%d ", get_card(&deck));
    }
    printf("\n");
    return 0;
}
#ifdef UNIT_TEST
int main()
{
    test_card();
    test_rank();
    return 0;
}
#endif
