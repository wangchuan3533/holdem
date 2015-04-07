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

    assert(ranks[0].level == ROYAL_FLUSH && ranks[0].score == ((((12 * 13) + 11) * 13 + 10) * 13 + 9) * 13 + 8);
    assert(ranks[1].level == STRAIGHT_FLUSH && ranks[1].score == ((((8 * 13) + 7) * 13 + 6) * 13 + 5) * 13 + 4);
    assert(ranks[2].level == FOUR_OF_A_KIND && ranks[2].score == 10 * 13 + 3);
    assert(ranks[3].level == FULL_HORSE && ranks[3].score == 9 * 13 + 12);
    assert(ranks[4].level == FLUSH && ranks[4].score == ((((12 * 13) + 8) * 13 + 6) * 13 + 3) * 13 + 2);
    assert(ranks[5].level == THREE_OF_A_KIND && ranks[5].score == ((2 * 13) + 12) * 13 + 9);

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
