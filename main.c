#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "bashgraphics.h"
#include "game.h"

static void shutdown_hook()
{
    printf(ASCII_CLEAR);
    printf(ASCII_RESET_CURSOR);
}

int main(int argc, char** argv)
{
    char* cards_file_path = 0;
    bool are_args_valid = true;
    
    if (argc >= 2)
        cards_file_path = argv[1];

    if (argc < 0 || !cards_file_path)
    {
        printf("No card file specified. Run the program again specifying the cards file:\n");
        printf("%s [CARDS_FILE_PATH]\n", argv[0]);
        exit(1);
    }

    FILE* cards_file = fopen(cards_file_path, "r");
    if (!cards_file)
    {
        printf("Failed to open cards file '%s'\n", cards_file_path);
        exit(1);
    }

    CardList cards = parse_cards_file(cards_file);
    fclose(cards_file);

    set_ingame_sigint_handler();

    struct winsize console_size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &console_size);
        
    Canvas canvas = create_canvas(console_size.ws_col, console_size.ws_row);

    set_shutdown_hook(&shutdown_hook);

    printf("Welcome to The Game. Let's begin.\n\n");
    srand(time(0));

    bool is_first_loop = true;
    while(true)
    {
        if (!is_first_loop)
        {
            printf(ASCII_CLEAR);
            printf(ASCII_RESET_CURSOR);
        }

        printf("Press ENTER when you are ready for a card.\n");

        char response = 0;
        do
            response = getc(stdin);
        while (response != '\n');

        ioctl(STDOUT_FILENO, TIOCGWINSZ, &console_size);
        if (console_size.ws_col != canvas.width || console_size.ws_row != canvas.height)
        {
            free_canvas(&canvas);
            canvas = create_canvas(console_size.ws_col, console_size.ws_row);
        }

        Card card = choose_card(cards);
        
        printf(ASCII_CLEAR);
        printf(ASCII_RESET_CURSOR);

        canvas_clear_static(&canvas);
        
        for (size_t i = 0; i < canvas.width; ++i)
            canvas_putc_static(&canvas, '-', 0, i);

        canvas_printf_static(&canvas, 1, 0, "You will have %d seconds to complete the following prompt:", card.seconds);

        size_t prompt_len = strlen(cards.prompt_buf + card.prompt_offset);
        size_t prompt_extra_lines = prompt_len / canvas.width;
        
        canvas_printf_static(&canvas, 3, 0, "%s", cards.prompt_buf + card.prompt_offset);

        for (size_t i = 0; i < canvas.width; ++i)
            canvas_putc_static(&canvas, '-', 4 + prompt_extra_lines, i);

        draw_canvas(&canvas);

        countdown(&canvas, COUNTDOWN_SECS, "Get ready...");
        set_ingame_sigint_handler();

        countdown(&canvas, card.seconds, "GO!");
        set_ingame_sigint_handler();

        is_first_loop = false;
    }

    // These frees will never be reached, but it is a good reminder that this is memory that has
    // been allocated

    free_card_list(cards);
    free_canvas(&canvas);

    return 0;
}
