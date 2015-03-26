#ifndef _CARD_H
#define _CARD_H
typedef int card_t;
const char *card_to_string(card_t card);

typedef struct deck_s {
    card_t cards[52];
    int end;
} deck_t;
void init_deck(deck_t *deck);
int get_card(deck_t *deck);
#endif
