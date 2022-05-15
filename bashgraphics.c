#include "bashgraphics.h"

Canvas create_canvas(uint16_t width, uint16_t height)
{
    uint32_t buffer_size = width * height;
    char* buffer = malloc(buffer_size * 2);

    if (!buffer)
    {
        printf("malloc failure\n");
        exit(1);
    }

    Canvas canvas = {
        .width = width,
        .height = height,
        .static_buf = buffer,
        .transient_buf = buffer + (buffer_size),
    };

    memset(canvas.static_buf, 0, buffer_size);
    memset(canvas.transient_buf, ' ', buffer_size);

    return canvas;
}

void free_canvas(Canvas* canvas)
{
    // Static buffer and transient buffer were allocated together
    free(canvas->static_buf);
}

// TODO: Handle wrap arounds
void draw_canvas(Canvas* canvas)
{
    printf(ASCII_CLEAR);

    printf(ASCII_RESET_CURSOR);
    for (uint16_t row = 0; row < canvas->height; ++row)
    {
        for (uint16_t col = 0; col < canvas->width; ++col)
        {
            char c = *(canvas->transient_buf + (row * canvas->width) + col);
            putc(c, stdout);
        }

        if (row != canvas->height - 1)
            putc('\n', stdout);
    }

    printf(ASCII_RESET_CURSOR);
    for (uint16_t row = 0; row < canvas->height; ++row)
    {
        for (uint16_t col = 0; col < canvas->width; ++col)
        {
            char c = *(canvas->static_buf + (row * canvas->width) + col);

            if (c != 0)
                putc(c, stdout);
        }
        
        if (row != canvas->height - 1)
            putc('\n', stdout);
    }
}
