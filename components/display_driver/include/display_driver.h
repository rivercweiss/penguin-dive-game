#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "driver/spi_master.h"

#define DISPLAY_WIDTH 135
#define DISPLAY_HEIGHT 240
#define DISPLAY_COLOR_DEPTH 16

// ST7789 specific definitions
#define ST7789_SPI_HOST SPI2_HOST
#define ST7789_SPI_CLOCK_SPEED 40000000  // 40MHz
#define ST7789_DC_PIN GPIO_NUM_23
#define ST7789_RST_PIN GPIO_NUM_18
#define ST7789_CS_PIN GPIO_NUM_5
#define ST7789_SCLK_PIN GPIO_NUM_13
#define ST7789_MOSI_PIN GPIO_NUM_15

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
    // Hardware-specific fields
    spi_device_handle_t spi_handle;
    bool hardware_initialized;
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

// Hardware-specific functions
bool st7789_init(display_context_t* ctx);
void st7789_send_command(display_context_t* ctx, uint8_t cmd);
void st7789_send_data(display_context_t* ctx, const uint8_t* data, size_t len);
void st7789_set_window(display_context_t* ctx, uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);
void st7789_write_framebuffer(display_context_t* ctx);