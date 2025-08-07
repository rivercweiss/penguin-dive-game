#include "display_driver.h"
#include <stdlib.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"

bool display_driver_init(display_context_t* ctx) {
    if (!ctx) return false;
    
    memset(ctx, 0, sizeof(display_context_t));
    
    ctx->width = DISPLAY_WIDTH;
    ctx->height = DISPLAY_HEIGHT;
    
    // Allocate framebuffer
    size_t buffer_size = ctx->width * ctx->height * sizeof(uint16_t);
    ctx->framebuffer = malloc(buffer_size);
    if (!ctx->framebuffer) {
        return false;
    }
    
    // Allocate back buffer for double buffering
    ctx->back_buffer = malloc(buffer_size);
    if (!ctx->back_buffer) {
        free(ctx->framebuffer);
        ctx->framebuffer = NULL;
        return false;
    }
    
    ctx->double_buffered = true;
    
    // Clear both buffers
    memset(ctx->framebuffer, 0, buffer_size);
    memset(ctx->back_buffer, 0, buffer_size);
    
    ctx->initialized = true;
    
    // Initialize hardware (ST7789 display)
    if (!st7789_init(ctx)) {
        ESP_LOGE("DISPLAY", "Failed to initialize ST7789 hardware");
        // Don't fail completely - allow software-only operation for testing
    }
    
    return true;
}

void display_driver_deinit(display_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->framebuffer) {
        free(ctx->framebuffer);
        ctx->framebuffer = NULL;
    }
    
    if (ctx->back_buffer) {
        free(ctx->back_buffer);
        ctx->back_buffer = NULL;
    }
    
    ctx->initialized = false;
}

void display_driver_clear_screen(display_context_t* ctx, display_color_t color) {
    if (!ctx || !ctx->initialized || !ctx->back_buffer) return;
    
    uint16_t* buffer = ctx->back_buffer;
    size_t pixel_count = ctx->width * ctx->height;
    
    for (size_t i = 0; i < pixel_count; i++) {
        buffer[i] = color;
    }
    
    // Also clear the front buffer to ensure consistency
    if (ctx->framebuffer) {
        uint16_t* front_buffer = ctx->framebuffer;
        for (size_t i = 0; i < pixel_count; i++) {
            front_buffer[i] = color;
        }
    }
}

void display_driver_draw_rectangle(display_context_t* ctx, int x, int y, int width, int height, display_color_t color) {
    if (!ctx || !ctx->initialized || !ctx->back_buffer) return;
    
    // Clamp to screen bounds
    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    if (x + width > ctx->width) {
        width = ctx->width - x;
    }
    if (y + height > ctx->height) {
        height = ctx->height - y;
    }
    
    if (width <= 0 || height <= 0) return;
    
    uint16_t* buffer = ctx->back_buffer;
    
    // Ensure row-major order: buffer[y * width + x]
    for (int row = y; row < y + height; row++) {
        for (int col = x; col < x + width; col++) {
            int index = row * ctx->width + col;
            if (index >= 0 && index < ctx->width * ctx->height) {
                buffer[index] = color;
            }
        }
    }
}

void display_driver_draw_sprite(display_context_t* ctx, sprite_t* sprite) {
    if (!sprite) return;
    
    display_driver_draw_rectangle(ctx, sprite->x, sprite->y, sprite->width, sprite->height, sprite->color);
}

// Optimized 5x7 bitmap font for better readability on low pixel count displays
static const uint8_t font_bitmaps[][7] = {
    // Space (ASCII 32)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ! (ASCII 33)
    {0x10, 0x10, 0x10, 0x10, 0x00, 0x10, 0x00},
    // " (ASCII 34)
    {0x28, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00},
    // # (ASCII 35)
    {0x28, 0x7C, 0x28, 0x7C, 0x28, 0x00, 0x00},
    // $ (ASCII 36)
    {0x10, 0x3C, 0x50, 0x38, 0x14, 0x78, 0x10},
    // % (ASCII 37)
    {0x60, 0x64, 0x08, 0x10, 0x26, 0x06, 0x00},
    // & (ASCII 38)
    {0x30, 0x48, 0x30, 0x4A, 0x44, 0x3A, 0x00},
    // ' (ASCII 39)
    {0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ( (ASCII 40)
    {0x08, 0x10, 0x20, 0x20, 0x20, 0x10, 0x08},
    // ) (ASCII 41)
    {0x20, 0x10, 0x08, 0x08, 0x08, 0x10, 0x20},
    // * (ASCII 42)
    {0x10, 0x54, 0x38, 0x7C, 0x38, 0x54, 0x10},
    // + (ASCII 43)
    {0x00, 0x10, 0x10, 0x7C, 0x10, 0x10, 0x00},
    // , (ASCII 44)
    {0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x20},
    // - (ASCII 45)
    {0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00},
    // . (ASCII 46)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00},
    // / (ASCII 47)
    {0x04, 0x08, 0x10, 0x20, 0x40, 0x00, 0x00},
    // 0 (ASCII 48)
    {0x38, 0x44, 0x44, 0x44, 0x44, 0x44, 0x38},
    // 1 (ASCII 49)
    {0x10, 0x30, 0x10, 0x10, 0x10, 0x10, 0x38},
    // 2 (ASCII 50)
    {0x38, 0x44, 0x04, 0x08, 0x10, 0x20, 0x7C},
    // 3 (ASCII 51)
    {0x38, 0x44, 0x04, 0x18, 0x04, 0x44, 0x38},
    // 4 (ASCII 52)
    {0x08, 0x18, 0x28, 0x48, 0x7C, 0x08, 0x08},
    // 5 (ASCII 53)
    {0x7C, 0x40, 0x78, 0x04, 0x04, 0x44, 0x38},
    // 6 (ASCII 54)
    {0x18, 0x20, 0x40, 0x78, 0x44, 0x44, 0x38},
    // 7 (ASCII 55)
    {0x7C, 0x04, 0x08, 0x10, 0x10, 0x10, 0x10},
    // 8 (ASCII 56)
    {0x38, 0x44, 0x44, 0x38, 0x44, 0x44, 0x38},
    // 9 (ASCII 57)
    {0x38, 0x44, 0x44, 0x3C, 0x04, 0x08, 0x30},
    // : (ASCII 58)
    {0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00},
    // ; (ASCII 59)
    {0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x20},
    // < (ASCII 60)
    {0x08, 0x10, 0x20, 0x40, 0x20, 0x10, 0x08},
    // = (ASCII 61)
    {0x00, 0x00, 0x7C, 0x00, 0x7C, 0x00, 0x00},
    // > (ASCII 62)
    {0x20, 0x10, 0x08, 0x04, 0x08, 0x10, 0x20},
    // ? (ASCII 63)
    {0x38, 0x44, 0x04, 0x08, 0x10, 0x00, 0x10},
    // @ (ASCII 64)
    {0x38, 0x44, 0x5C, 0x54, 0x5C, 0x40, 0x38},
    // A (ASCII 65)
    {0x38, 0x44, 0x44, 0x7C, 0x44, 0x44, 0x44},
    // B (ASCII 66)
    {0x78, 0x44, 0x44, 0x78, 0x44, 0x44, 0x78},
    // C (ASCII 67)
    {0x38, 0x44, 0x40, 0x40, 0x40, 0x44, 0x38},
    // D (ASCII 68)
    {0x78, 0x44, 0x44, 0x44, 0x44, 0x44, 0x78},
    // E (ASCII 69)
    {0x7C, 0x40, 0x40, 0x78, 0x40, 0x40, 0x7C},
    // F (ASCII 70)
    {0x7C, 0x40, 0x40, 0x78, 0x40, 0x40, 0x40},
    // G (ASCII 71)
    {0x38, 0x44, 0x40, 0x5C, 0x44, 0x44, 0x38},
    // H (ASCII 72)
    {0x44, 0x44, 0x44, 0x7C, 0x44, 0x44, 0x44},
    // I (ASCII 73)
    {0x38, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38},
    // J (ASCII 74)
    {0x1C, 0x08, 0x08, 0x08, 0x08, 0x48, 0x30},
    // K (ASCII 75)
    {0x44, 0x48, 0x50, 0x60, 0x50, 0x48, 0x44},
    // L (ASCII 76)
    {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7C},
    // M (ASCII 77)
    {0x44, 0x6C, 0x54, 0x44, 0x44, 0x44, 0x44},
    // N (ASCII 78)
    {0x44, 0x64, 0x54, 0x4C, 0x44, 0x44, 0x44},
    // O (ASCII 79)
    {0x38, 0x44, 0x44, 0x44, 0x44, 0x44, 0x38},
    // P (ASCII 80)
    {0x78, 0x44, 0x44, 0x78, 0x40, 0x40, 0x40},
    // Q (ASCII 81)
    {0x38, 0x44, 0x44, 0x44, 0x54, 0x48, 0x34},
    // R (ASCII 82)
    {0x78, 0x44, 0x44, 0x78, 0x50, 0x48, 0x44},
    // S (ASCII 83)
    {0x38, 0x44, 0x40, 0x38, 0x04, 0x44, 0x38},
    // T (ASCII 84)
    {0x7C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10},
    // U (ASCII 85)
    {0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x38},
    // V (ASCII 86)
    {0x44, 0x44, 0x44, 0x44, 0x28, 0x28, 0x10},
    // W (ASCII 87)
    {0x44, 0x44, 0x44, 0x54, 0x6C, 0x44, 0x44},
    // X (ASCII 88)
    {0x44, 0x28, 0x10, 0x10, 0x10, 0x28, 0x44},
    // Y (ASCII 89)
    {0x44, 0x28, 0x28, 0x10, 0x10, 0x10, 0x10},
    // Z (ASCII 90)
    {0x7C, 0x04, 0x08, 0x10, 0x20, 0x40, 0x7C},
    // [ (ASCII 91)
    {0x38, 0x20, 0x20, 0x20, 0x20, 0x20, 0x38},
    // \ (ASCII 92)
    {0x40, 0x20, 0x10, 0x08, 0x04, 0x00, 0x00},
    // ] (ASCII 93)
    {0x38, 0x08, 0x08, 0x08, 0x08, 0x08, 0x38},
    // ^ (ASCII 94)
    {0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x00},
    // _ (ASCII 95)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C},
    // ` (ASCII 96)
    {0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00},
    // a (ASCII 97)
    {0x00, 0x00, 0x38, 0x04, 0x3C, 0x44, 0x3C},
    // b (ASCII 98)
    {0x40, 0x40, 0x78, 0x44, 0x44, 0x44, 0x78},
    // c (ASCII 99)
    {0x00, 0x00, 0x38, 0x44, 0x40, 0x44, 0x38},
    // d (ASCII 100)
    {0x04, 0x04, 0x3C, 0x44, 0x44, 0x44, 0x3C},
    // e (ASCII 101)
    {0x00, 0x00, 0x38, 0x44, 0x7C, 0x40, 0x38},
    // f (ASCII 102)
    {0x18, 0x24, 0x20, 0x70, 0x20, 0x20, 0x20},
    // g (ASCII 103)
    {0x3C, 0x44, 0x44, 0x3C, 0x04, 0x44, 0x38},
    // h (ASCII 104)
    {0x40, 0x40, 0x78, 0x44, 0x44, 0x44, 0x44},
    // i (ASCII 105)
    {0x10, 0x00, 0x30, 0x10, 0x10, 0x10, 0x38},
    // j (ASCII 106)
    {0x08, 0x00, 0x18, 0x08, 0x08, 0x48, 0x30},
    // k (ASCII 107)
    {0x40, 0x40, 0x44, 0x48, 0x70, 0x48, 0x44},
    // l (ASCII 108)
    {0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38},
    // m (ASCII 109)
    {0x00, 0x00, 0x6C, 0x54, 0x54, 0x54, 0x54},
    // n (ASCII 110)
    {0x00, 0x00, 0x78, 0x44, 0x44, 0x44, 0x44},
    // o (ASCII 111)
    {0x00, 0x00, 0x38, 0x44, 0x44, 0x44, 0x38},
    // p (ASCII 112)
    {0x78, 0x44, 0x44, 0x78, 0x40, 0x40, 0x40},
    // q (ASCII 113)
    {0x3C, 0x44, 0x44, 0x3C, 0x04, 0x04, 0x04},
    // r (ASCII 114)
    {0x00, 0x00, 0x58, 0x64, 0x40, 0x40, 0x40},
    // s (ASCII 115)
    {0x00, 0x00, 0x3C, 0x40, 0x38, 0x04, 0x78},
    // t (ASCII 116)
    {0x20, 0x20, 0x70, 0x20, 0x20, 0x24, 0x18},
    // u (ASCII 117)
    {0x00, 0x00, 0x44, 0x44, 0x44, 0x44, 0x3C},
    // v (ASCII 118)
    {0x00, 0x00, 0x44, 0x44, 0x28, 0x28, 0x10},
    // w (ASCII 119)
    {0x00, 0x00, 0x44, 0x54, 0x54, 0x6C, 0x44},
    // x (ASCII 120)
    {0x00, 0x00, 0x44, 0x28, 0x10, 0x28, 0x44},
    // y (ASCII 121)
    {0x00, 0x00, 0x44, 0x44, 0x3C, 0x04, 0x38},
    // z (ASCII 122)
    {0x00, 0x00, 0x7C, 0x08, 0x10, 0x20, 0x7C},
    // { (ASCII 123)
    {0x18, 0x10, 0x10, 0x20, 0x10, 0x10, 0x18},
    // | (ASCII 124)
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10},
    // } (ASCII 125)
    {0x30, 0x10, 0x10, 0x08, 0x10, 0x10, 0x30},
    // ~ (ASCII 126)
    {0x00, 0x20, 0x54, 0x08, 0x00, 0x00, 0x00}
};

void display_driver_draw_text(display_context_t* ctx, int x, int y, const char* text, display_color_t color) {
    if (!ctx || !ctx->initialized || !text) return;
    
    const int char_width = 5;
    const int char_height = 7;
    
    int current_x = x;
    int current_y = y;
    
    while (*text) {
        if (*text == '\n') {
            current_x = x;
            current_y += char_height + 1;
        } else if (*text >= 32 && *text <= 126) {
            // Get the bitmap for this character
            const uint8_t* bitmap = font_bitmaps[*text - 32];
            
            // Draw the character pixel by pixel
            for (int row = 0; row < char_height; row++) {
                uint8_t row_data = bitmap[row];
                for (int col = 0; col < char_width; col++) {
                    if (row_data & (0x40 >> col)) {
                        display_driver_set_pixel(ctx, current_x + col, current_y + row, color);
                    }
                }
            }
            current_x += char_width + 1;
        }
        text++;
    }
}

void display_driver_swap_buffers(display_context_t* ctx) {
    if (!ctx || !ctx->initialized || !ctx->framebuffer || !ctx->back_buffer) return;
    
    // Swap the buffers
    uint16_t* temp = ctx->framebuffer;
    ctx->framebuffer = ctx->back_buffer;
    ctx->back_buffer = temp;
}

void display_driver_flush(display_context_t* ctx) {
    if (!ctx || !ctx->initialized) return;
    
    // Send the framebuffer to the physical display if hardware is initialized
    if (ctx->hardware_initialized) {
        st7789_write_framebuffer(ctx);
    }
}

uint16_t display_driver_get_pixel(display_context_t* ctx, int x, int y) {
    if (!ctx || !ctx->initialized || !ctx->framebuffer) return 0;
    
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) {
        return 0;
    }
    
    int index = y * ctx->width + x;
    if (index >= 0 && index < ctx->width * ctx->height) {
        return ctx->framebuffer[index];
    }
    return 0;
}

void display_driver_set_pixel(display_context_t* ctx, int x, int y, display_color_t color) {
    if (!ctx || !ctx->initialized || !ctx->back_buffer) return;
    
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) {
        return;
    }
    
    int index = y * ctx->width + x;
    if (index >= 0 && index < ctx->width * ctx->height) {
        ctx->back_buffer[index] = color;
    }
}

bool display_driver_is_initialized(display_context_t* ctx) {
    if (!ctx) return false;
    return ctx->initialized && ctx->framebuffer && ctx->back_buffer;
}

// ST7789 Hardware-specific functions

static const char* TAG = "ST7789";

bool st7789_init(display_context_t* ctx) {
    if (!ctx) return false;
    
    ESP_LOGI(TAG, "Initializing ST7789 display...");
    
    // Configure GPIO pins
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << ST7789_DC_PIN) | (1ULL << ST7789_RST_PIN) | (1ULL << ST7789_CS_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    
    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = ST7789_MOSI_PIN,
        .miso_io_num = -1,  // Not used
        .sclk_io_num = ST7789_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2,
    };
    
    esp_err_t ret = spi_bus_initialize(ST7789_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus initialization failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Configure SPI device
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = ST7789_SPI_CLOCK_SPEED,
        .mode = 0,  // SPI mode 0
        .spics_io_num = ST7789_CS_PIN,
        .queue_size = 7,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };
    
    ret = spi_bus_add_device(ST7789_SPI_HOST, &devcfg, &ctx->spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI device initialization failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Hardware reset
    gpio_set_level(ST7789_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(ST7789_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Initialize display with commands
    st7789_send_command(ctx, 0x11);  // Sleep out
    vTaskDelay(pdMS_TO_TICKS(120));
    
    st7789_send_command(ctx, 0x3A);  // Color mode
    uint8_t color_mode = 0x55;  // 16-bit color
    st7789_send_data(ctx, &color_mode, 1);
    
    st7789_send_command(ctx, 0x36);  // Memory access control
    uint8_t madctl = 0x00;  // Normal orientation
    st7789_send_data(ctx, &madctl, 1);
    
    st7789_send_command(ctx, 0x2A);  // Column address set
    uint8_t caset[4] = {0x00, 0x00, 0x00, 0x86};  // 0-134 (135 pixels)
    st7789_send_data(ctx, caset, 4);
    
    st7789_send_command(ctx, 0x2B);  // Row address set
    uint8_t raset[4] = {0x00, 0x00, 0x00, 0xEF};  // 0-239 (240 pixels)
    st7789_send_data(ctx, raset, 4);
    
    st7789_send_command(ctx, 0x29);  // Display on
    
    ctx->hardware_initialized = true;
    ESP_LOGI(TAG, "ST7789 display initialized successfully");
    return true;
}

void st7789_send_command(display_context_t* ctx, uint8_t cmd) {
    if (!ctx || !ctx->hardware_initialized) return;
    
    gpio_set_level(ST7789_DC_PIN, 0);  // Command mode
    
    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    
    esp_err_t ret = spi_device_transmit(ctx->spi_handle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI command transmission failed: %s", esp_err_to_name(ret));
    }
}

void st7789_send_data(display_context_t* ctx, const uint8_t* data, size_t len) {
    if (!ctx || !ctx->hardware_initialized || !data) return;
    
    gpio_set_level(ST7789_DC_PIN, 1);  // Data mode
    
    spi_transaction_t trans = {
        .length = len * 8,
        .tx_buffer = data,
    };
    
    esp_err_t ret = spi_device_transmit(ctx->spi_handle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI data transmission failed: %s", esp_err_to_name(ret));
    }
}

void st7789_set_window(display_context_t* ctx, uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end) {
    if (!ctx || !ctx->hardware_initialized) return;
    
    st7789_send_command(ctx, 0x2A);  // Column address set
    uint8_t caset[4] = {
        (x_start >> 8) & 0xFF,
        x_start & 0xFF,
        (x_end >> 8) & 0xFF,
        x_end & 0xFF
    };
    st7789_send_data(ctx, caset, 4);
    
    st7789_send_command(ctx, 0x2B);  // Row address set
    uint8_t raset[4] = {
        (y_start >> 8) & 0xFF,
        y_start & 0xFF,
        (y_end >> 8) & 0xFF,
        y_end & 0xFF
    };
    st7789_send_data(ctx, raset, 4);
    
    st7789_send_command(ctx, 0x2C);  // Memory write
}

void st7789_write_framebuffer(display_context_t* ctx) {
    if (!ctx || !ctx->hardware_initialized || !ctx->framebuffer) return;
    
    // Set window to full screen
    st7789_set_window(ctx, 0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    
    // Send framebuffer data
    gpio_set_level(ST7789_DC_PIN, 1);  // Data mode
    
    spi_transaction_t trans = {
        .length = DISPLAY_WIDTH * DISPLAY_HEIGHT * 16,  // 16 bits per pixel
        .tx_buffer = ctx->framebuffer,
    };
    
    esp_err_t ret = spi_device_transmit(ctx->spi_handle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Framebuffer transmission failed: %s", esp_err_to_name(ret));
    }
}