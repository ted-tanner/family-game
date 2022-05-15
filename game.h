#ifndef __GAME_H
#define __GAME_H

#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "bashgraphics.h"

#define COUNTDOWN_SECS 4

#define ASCII_CLEAR "\033[2J"
#define ASCII_RESET_CURSOR "\033[H"

typedef struct _card {
    size_t prompt_offset;
    int32_t seconds;
} Card;

typedef struct _card_list {
    Card* card_buf;
    char* prompt_buf;
    int64_t prompt_buf_size;
    int64_t card_count;
} CardList;

typedef struct {
    bool countdown_continue;
    bool awaiting_response_to_quit;
    void (*shutdown_hook)();
} GameState;

CardList parse_cards_file(FILE* file);
void free_card_list(CardList list);
Card choose_card(CardList cards);
void countdown(Canvas* canvas, int32_t time, char* message);
void set_ingame_sigint_handler();
void set_shutdown_hook(void (*func)());

#endif
