#include "game.h"

static GameState game_state = {0};

CardList parse_cards_file(FILE* file)
{
    // Find longest line and count number of lines
    // NOTE: Memory footprint can be shrunk by only setting the buffer to the longest
    //       length of text not in a comment and before a tilde (~)
    int64_t line_count = 0;
    int32_t max_line_length = 0;
    int32_t curr_line_length = 0;
    for(char curr = fgetc(file); curr != EOF; curr = fgetc(file), ++curr_line_length)
    {
        if (curr == '\n')
        {
            ++line_count;

            if (curr_line_length > max_line_length)
                max_line_length = curr_line_length;

            curr_line_length = -1;
        }
    }
    
    rewind(file);

    // Add a byte for terminating 0
    ++max_line_length;

    char* prompt_buffer = malloc(max_line_length * line_count);
    Card* card_buffer = malloc(sizeof(Card) * line_count);

    if (!prompt_buffer || !card_buffer)
    {
        printf("malloc failure\n");
        exit(1);
    }

    int64_t pos_in_line = 0;
    int64_t line_number = 0;

    Card* card_buffer_cursor = card_buffer;

    bool hit_tilde = false;
    int64_t tilde_pos_in_line = 0;
 
    for(char curr = fgetc(file); curr != EOF; curr = fgetc(file), ++pos_in_line)
    {
        if (curr == '#' && pos_in_line == 0)
        {
            pos_in_line = -1;
            --line_count;

            do
            {
                if (curr == EOF)
                    goto end_of_outer_loop; // Break out of outer loop
                
                curr = fgetc(file);
            }
            while (curr != '\n');

            continue;
        }
        else if (curr == '\n')
        {
            *(prompt_buffer + max_line_length * line_number + pos_in_line) = 0;
                        
            if (!hit_tilde)
            {
                // Ignore line
                --line_count;
                --line_number;
            }
            else
            {
                int32_t seconds = strtol(prompt_buffer
                                         + max_line_length * line_number
                                         + tilde_pos_in_line
                                         + 1,
                                         0, 10);
                if (seconds == 0)
                {
                    printf("Invalid line in game file. The time to perform the action was invalid ");
                    printf("for the following action:\n\"%s\"\n", prompt_buffer + max_line_length * line_number);
                    exit(1);
                }

                if (seconds < 0)
                {
                    seconds = -seconds;
                }
                    
                Card card = {
                    .prompt_offset = max_line_length * line_number,
                    .seconds = seconds,
                };

                *card_buffer_cursor = card;
                ++card_buffer_cursor;
            }

            ++line_number;
            pos_in_line = -1;
            
            hit_tilde = false;
            
            continue;
        }
        else if (curr == '~')
        {
            hit_tilde = true;
            tilde_pos_in_line = pos_in_line;

            char* curr_pos = prompt_buffer + max_line_length * line_number + pos_in_line;
            *(curr_pos) = 0;
            
            // Remove trailing space from prompt
            if (*(curr_pos - 1) == ' ' || *(curr_pos - 1) == '\t')
                *(curr_pos - 1) = 0;
        }

        *(prompt_buffer + max_line_length * line_number + pos_in_line) = curr;
    }
end_of_outer_loop:

    *(prompt_buffer + max_line_length * line_number + pos_in_line) = 0;

    CardList cards = {
        .card_buf = realloc(card_buffer, sizeof(Card) * line_count),
        .prompt_buf = realloc(prompt_buffer, max_line_length * line_count),
        .prompt_buf_size = max_line_length,
        .card_count = line_count,
    };
    
    return cards;
}

void free_card_list(CardList list)
{
    free(list.card_buf);
    free(list.prompt_buf);
}

Card choose_card(CardList cards)
{
    int32_t rand_card_index = (rand() ^ time(0)) % cards.card_count;
    return *(cards.card_buf + rand_card_index);
}

static void countdown_sigint_handler(int num)
{
    game_state.countdown_continue = false;
}

void countdown(Canvas* canvas, int32_t time, char* message)
{
    const int64_t checks_per_second = 60;
    
    signal(SIGINT, countdown_sigint_handler);

    uint16_t canvas_center_row = canvas->height / 2;
    uint16_t canvas_center_col = canvas->width / 2;

    // Timer is in format 00:00, so offset is half of that lengthz
    uint16_t timer_offset = canvas->width >= 3 ? 3 : 0;

    char timer_stop_message[] = "Press Ctrl + C to stop the timer";
    size_t timer_stop_message_offset = strlen(timer_stop_message) / 2;
    if (canvas->width <= timer_stop_message_offset)
        timer_stop_message_offset = 0;

    int32_t seconds = time;

    game_state.countdown_continue = true;
    for (uint8_t check_count = 0; seconds >= 0 && game_state.countdown_continue; --check_count)
    {
        // TODO: Account for the time it takes to execute the following block
        if (check_count == 0)
        {
            canvas_clear_transitory(canvas);
            
            size_t message_offset = strlen(message) / 2 + 1;
            if (canvas->width <= message_offset)
                message_offset = 0;

            canvas_printf_transitory(canvas, canvas_center_row, canvas_center_col - message_offset, message);
            
            int32_t minutes = seconds / 60;
            int32_t second_less_minutes = seconds - (60 * minutes);
            
            canvas_printf_transitory(canvas,
                                     canvas_center_row + 2,
                                     canvas_center_col - timer_offset,
                                     "%02d:%02d\n",
                                     minutes, second_less_minutes);

            canvas_printf_transitory(canvas,
                                     canvas_center_row + 5,
                                     canvas_center_col - timer_stop_message_offset,
                                     timer_stop_message);

            draw_canvas(canvas);

            --seconds;
            check_count = checks_per_second + 1;
        }
            
        usleep((1.0 / (float) checks_per_second) * 1000000);
    }
}

static void ingame_sigint_handler(int num)
{
    if (!game_state.awaiting_response_to_quit)
    {
        game_state.awaiting_response_to_quit = true;

        printf(ASCII_CLEAR);
        printf(ASCII_RESET_CURSOR);
        
        printf("Are you sure you want to quit? (y/n) ");
        char response = getc(stdin);

        if (response == 'y')
        {
            if (game_state.shutdown_hook != 0)
                (*game_state.shutdown_hook)();
            exit(0);
        }

        printf("\nPress ENTER when you are ready for a card.\n");

        game_state.awaiting_response_to_quit = false;
    }
}

void set_ingame_sigint_handler()
{
    signal(SIGINT, ingame_sigint_handler);
}

void set_shutdown_hook(void (*func)())
{
    game_state.shutdown_hook = func;
}
