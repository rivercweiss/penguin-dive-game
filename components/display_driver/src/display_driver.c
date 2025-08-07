#include "display_driver.h"
#include <stdlib.h>
#include <string.h>

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

void display_driver_draw_text(display_context_t* ctx, int x, int y, const char* text, display_color_t color) {
    if (!ctx || !ctx->initialized || !text) return;
    
    // Simple text rendering - each character is 8x8 pixels
    const int char_width = 8;
    const int char_height = 8;
    
    int current_x = x;
    int current_y = y;
    
    while (*text) {
        if (*text == '\n') {
            current_x = x;
            current_y += char_height;
        } else {
            // Draw a simple rectangle for each character (placeholder)
            display_driver_draw_rectangle(ctx, current_x, current_y, char_width, char_height, color);
            current_x += char_width;
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
    
    // In a real implementation, this would send the framebuffer to the physical display
    // For testing purposes, this is a no-op
}

uint16_t display_driver_get_pixel(display_context_t* ctx, int x, int y) {
    if (!ctx || !ctx->initialized || !ctx->back_buffer) return 0;
    
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) {
        return 0;
    }
    
    int index = y * ctx->width + x;
    if (index >= 0 && index < ctx->width * ctx->height) {
        return ctx->back_buffer[index];
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