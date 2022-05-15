#ifndef __BASH_GRAPHICS_H
#define __BASH_GRAPHICS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASCII_CLEAR "\033[2J"
#define ASCII_RESET_CURSOR "\033[H"

#define CANVAS_PRINTF_BUF_SIZE 2048

typedef struct {
    uint16_t width;
    uint16_t height;
    char* static_buf;
    char* transient_buf;
} Canvas;

Canvas create_canvas(uint16_t width, uint16_t height);
void free_canvas(Canvas* canvas);
void draw_canvas(Canvas* canvas);

#define canvas_putc_static(canvas, item, row, col) if ((row) < (canvas)->height && (col) < (canvas)->width) \
        *((canvas)->static_buf + (row) * (canvas)->width + (col)) = (item);
#define canvas_putc_transitory(canvas, item, row, col) if ((row) < (canvas)->height && (col) < (canvas)->width) \
        *((canvas)->static_buf + (row) * (canvas)->width + (col)) = (item);

// TODO: Save part that would be overwritten by terminating 0 in a temp var
#define canvas_printf_static(canvas, row, col, item, ...) if ((row) < (canvas)->height && (col) < (canvas)->width) { \
        char buffer[CANVAS_PRINTF_BUF_SIZE];                            \
        int32_t copy_size = sprintf(buffer, (item), ##__VA_ARGS__);     \
        memcpy((canvas)->static_buf + (row) * (canvas)->width + (col), buffer, copy_size); }

#define canvas_printf_transitory(canvas, row, col, item, ...) if ((row) < (canvas)->height && (col) < (canvas)->width) { \
        char buffer[CANVAS_PRINTF_BUF_SIZE];                            \
        int32_t copy_size = sprintf(buffer, (item), ##__VA_ARGS__);     \
        memcpy((canvas)->transient_buf + (row) * (canvas)->width + (col), buffer, copy_size); }

// The static buffer has a "transparent" background. Slots that are set to 0 don't
// get printed (so the transient buffer can be shown behind it)
#define canvas_clear_static(canvas) memset((canvas)->static_buf, 0, (canvas)->width * (canvas)->height);
#define canvas_clear_transitory(canvas) memset((canvas)->transient_buf, ' ', (canvas)->width * (canvas)->height);

#define canvas_make_static_row_opaque(canvas, row) memset((canvas)->static_buf + (row) * (canvas)->width, \
                                                          ' ',          \
                                                          (canvas)->width);

#endif
