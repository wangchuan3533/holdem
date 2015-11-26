#include "holdem.h"
#include "card.h"
static const char *card_string[] = {
    "[♥2]",
    "[♥3]",
    "[♥4]",
    "[♥5]",
    "[♥6]",
    "[♥7]",
    "[♥8]",
    "[♥9]",
    "[♥10]",
    "[♥J]",
    "[♥Q]",
    "[♥K]",
    "[♥A]",
    "[♠2]",
    "[♠3]",
    "[♠4]",
    "[♠5]",
    "[♠6]",
    "[♠7]",
    "[♠8]",
    "[♠9]",
    "[♠10]",
    "[♠J]",
    "[♠Q]",
    "[♠K]",
    "[♠A]",
    "[♦2]",
    "[♦3]",
    "[♦4]",
    "[♦5]",
    "[♦6]",
    "[♦7]",
    "[♦8]",
    "[♦9]",
    "[♦10]",
    "[♦J]",
    "[♦Q]",
    "[♦K]",
    "[♦A]",
    "[♣2]",
    "[♣3]",
    "[♣4]",
    "[♣5]",
    "[♣6]",
    "[♣7]",
    "[♣8]",
    "[♣9]",
    "[♣10]",
    "[♣J]",
    "[♣Q]",
    "[♣K]",
    "[♣A]",
};

inline const char *card_to_string(card_t card)
{
    return card_string[card];
}

inline void init_deck(deck_t *deck)
{
    int i;
    srandom(time(NULL));
    for (i = 0; i < 52; i++) {
        deck->cards[i] = i;
    }
    deck->end = 52;
}

inline int get_card(deck_t *deck)
{
    int index = random() % (deck->end--);
    if (index == deck->end) {
        return deck->cards[deck->end];
    }
    deck->cards[index] ^= deck->cards[deck->end];
    deck->cards[deck->end] ^= deck->cards[index];
    deck->cards[index] ^= deck->cards[deck->end];
    return deck->cards[deck->end];
}
