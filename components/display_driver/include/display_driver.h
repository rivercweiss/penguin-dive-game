#pragma once

#include <stdbool.h>
#include <stdint.h>

// ESP-IDF specific includes only when building for ESP32
#ifdef ESP_PLATFORM
#include "driver/spi_master.h"
#include "driver/gpio.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Display specifications
#define DISPLAY_WIDTH  135
#define DISPLAY_HEIGHT 240

// M5StickC Plus display offsets for ST7789v2
#define DISPLAY_OFFSET_X 52
#define DISPLAY_OFFSET_Y 40

// SPI Pin definitions for M5StickC Plus ST7789v2 (ESP32 only)
#ifdef ESP_PLATFORM
#define TFT_MOSI_PIN   GPIO_NUM_15
#define TFT_CLK_PIN    GPIO_NUM_13
#define TFT_DC_PIN     GPIO_NUM_23
#define TFT_RST_PIN    GPIO_NUM_18
#define TFT_CS_PIN     GPIO_NUM_5
#define TFT_MISO_PIN   GPIO_NUM_12 // Set to actual MISO pin used on your hardware
#endif

// Color definitions (RGB565 format)
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0
#define COLOR_MAGENTA     0xF81F
#define COLOR_CYAN        0x07FF
#define COLOR_DARK_BLUE   0x0010
#define COLOR_ICE_BLUE    0x8F1F

// Forward declaration for LVGL display
struct _lv_display_t;

// Display context structure
typedef struct {
    struct _lv_display_t *lvgl_display; // LVGL display object
    bool initialized;
    
    // Legacy fields for compatibility (not used with LVGL)
    void *front_buffer;
    void *back_buffer;
    void *current_buffer;
} display_context_t;

// Display driver functions
bool display_driver_init(display_context_t *ctx);
void display_driver_deinit(display_context_t *ctx);
void display_driver_clear_screen(display_context_t *ctx, uint16_t color);
void display_driver_draw_rectangle(display_context_t *ctx, int x, int y, int width, int height, uint16_t color);
void display_driver_draw_text(display_context_t *ctx, int x, int y, const char *text, uint16_t color);
void display_driver_swap_buffers(display_context_t *ctx);
void display_driver_flush(display_context_t *ctx);
void display_driver_task_handler(void);
uint16_t display_driver_get_pixel(display_context_t *ctx, int x, int y); // For testing only

#ifdef __cplusplus
}
#endif