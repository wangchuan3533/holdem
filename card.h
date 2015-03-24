#ifndef _CARD_H
#define _CARD_H
typedef int card_t;
const char *card_to_string(card_t card);

typedef enum card_rank_e {
    HIGH_CARD,
    PAIR,
    TWO_PAIR,
    THREE_OF_A_KIND,
    STRAIGHT,
    FLUSH,
    FULL_HORSE,
    FOUR_OF_A_KIND,
    STRAIGHT_FLUSH,
} card_rank_t;

typedef struct deck_s {
    card_t cards[52];
    int offset;
} deck_t;
void init_deck(deck_t *deck);
int get_card(deck_t *deck);
#endif
