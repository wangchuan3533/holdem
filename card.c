#include <stdlib.h>
#include <time.h>
#include "card.h"
static const char *card_string[] = {
    "Heart 2",
    "Heart 3",
    "Heart 4",
    "Heart 5",
    "Heart 6",
    "Heart 7",
    "Heart 8",
    "Heart 9",
    "Heart 10",
    "Heart J",
    "Heart Q",
    "Heart K",
    "Heart A",
    "Spade 2",
    "Spade 3",
    "Spade 4",
    "Spade 5",
    "Spade 6",
    "Spade 7",
    "Spade 8",
    "Spade 9",
    "Spade 10",
    "Spade J",
    "Spade Q",
    "Spade K",
    "Spade A",
    "Diamond 2",
    "Diamond 3",
    "Diamond 4",
    "Diamond 5",
    "Diamond 6",
    "Diamond 7",
    "Diamond 8",
    "Diamond 9",
    "Diamond 10",
    "Diamond J",
    "Diamond Q",
    "Diamond K",
    "Diamond A",
    "Club 2",
    "Club 3",
    "Club 4",
    "Club 5",
    "Club 6",
    "Club 7",
    "Club 8",
    "Club 9",
    "Club 10",
    "Club J",
    "Club Q",
    "Club K",
    "Club A",
};

const char *card_to_string(card_t card)
{
    return card_string[card];
}

void init_deck(deck_t *deck)
{
    int i;
    srandom(time(NULL));
    for (i = 0; i < 52; i++) {
        deck->cards[i] = i;
    }
    deck->end = 52;
}

int get_card(deck_t *deck)
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
