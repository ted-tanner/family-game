#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

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

    bool is_first_loop = true;
    while(true)
    {
        if (!is_first_loop)
        {
            printf(ASCII_CLEAR);
            printf(ASCII_RESET_CURSOR);
        }

        printf("Press ENTER when you are ready for a card.\n");

        // Clear stdin and await response
        fseek(stdin, 0, SEEK_END);
        while (getc(stdin) != '\n');

        ioctl(STDOUT_FILENO, TIOCGWINSZ, &console_size);
        if (console_size.ws_col != canvas.width || console_size.ws_row != canvas.height)
        {
            free_canvas(&canvas);
            canvas = create_canvas(console_size.ws_col, console_size.ws_row);
        }

        Card card = choose_card(&cards);
        
        printf(ASCII_CLEAR);
        printf(ASCII_RESET_CURSOR);

        canvas_clear_static(&canvas);
        
        for (size_t i = 0; i < canvas.width; ++i)
            canvas_putc_static(&canvas, '-', 0, i);

        int32_t mins = card.seconds / 60;
        int32_t secs = card.seconds - mins * 60;

        if (mins == 0)
        {
            canvas_printf_static(&canvas,
                                 2,
                                 0,
                                 "You will have %d %s to complete the following prompt:",
                                 secs, secs == 1 ? "second" : "seconds");
        }
        else
        {
            if (secs == 0)
            {
                canvas_printf_static(&canvas,
                                     2,
                                     0,
                                     "You will have %d %s to complete the following prompt:",
                                     mins, mins == 1 ? "minute" : "minutes");
            }
            else
            {
                canvas_printf_static(&canvas,
                                     2,
                                     0,
                                     "You will have %d %s, %d %s to complete the following prompt:",
                                     mins, mins == 1 ? "minute" : "minutes",
                                     secs, secs == 1 ? "second" : "seconds");
            }
        }


        size_t prompt_len = strlen(cards.prompt_buf + card.prompt_offset);
        size_t prompt_extra_lines = prompt_len / canvas.width;

        canvas_printf_static(&canvas, 3, 0, ASCII_BOLD_ITALIC);
        canvas_printf_static(&canvas, 4, 0, "%s%s", cards.prompt_buf + card.prompt_offset, ASCII_CLEAR_FORMATTING);

        for (size_t i = 0; i < canvas.width; ++i)
            canvas_putc_static(&canvas, '-', 6 + prompt_extra_lines, i);

        draw_canvas(&canvas);

        countdown(&canvas, COUNTDOWN_SECS, "Get ready...");
        set_ingame_sigint_handler();

        countdown(&canvas, card.seconds, "GO!");
        set_ingame_sigint_handler();

        is_first_loop = false;
    }

    // These frees will never be reached, but it is a good reminder that this is memory that has
    // been allocated

    free_card_list(&cards);
    free_canvas(&canvas);

    return 0;
}
