#pragma once

#include <stdint.h>
#include <stdbool.h>

#define DISPLAY_WIDTH 135
#define DISPLAY_HEIGHT 240
#define DISPLAY_COLOR_DEPTH 16

typedef enum {
    COLOR_BLACK = 0x0000,
    COLOR_WHITE = 0xFFFF,
    COLOR_RED = 0xF800,
    COLOR_GREEN = 0x07E0,
    COLOR_BLUE = 0x001F,
    COLOR_YELLOW = 0xFFE0,
    COLOR_CYAN = 0x07FF,
    COLOR_MAGENTA = 0xF81F,
    COLOR_GRAY = 0x8410,
    COLOR_DARK_BLUE = 0x0010,
    COLOR_ICE_BLUE = 0xAEFB
} display_color_t;

typedef struct {
    bool initialized;
    uint16_t width;
    uint16_t height;
    uint16_t* framebuffer;
    bool double_buffered;
    uint16_t* back_buffer;
} display_context_t;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    display_color_t color;
} sprite_t;

bool display_driver_init(display_context_t* ctx);
void display_driver_deinit(display_context_t* ctx);
void display_driver_clear_screen(display_context_t* ctx, display_color_t color);
void display_driver_draw_rectangle(display_context_t* ctx, int x, int y, int width, int height, display_color_t color);
void display_driver_draw_sprite(display_context_t* ctx, sprite_t* sprite);
void display_driver_draw_text(display_context_t* ctx, int x, int y, const char* text, display_color_t color);
void display_driver_swap_buffers(display_context_t* ctx);
void display_driver_flush(display_context_t* ctx);
uint16_t display_driver_get_pixel(display_context_t* ctx, int x, int y);
void display_driver_set_pixel(display_context_t* ctx, int x, int y, display_color_t color);
bool display_driver_is_initialized(display_context_t* ctx);