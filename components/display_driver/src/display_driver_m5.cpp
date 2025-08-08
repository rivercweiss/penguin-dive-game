#include "display_driver.h"
#include "esp_log.h"

// Use M5Unified display stack
#include <M5Unified.h>

static const char *TAG = "display_driver_m5";

// Use an off-screen sprite as a back buffer to avoid flicker
static lgfx::LGFX_Sprite* s_sprite = nullptr;

extern "C" {

bool display_driver_init(display_context_t *ctx) {
    if (!ctx) {
        ESP_LOGE(TAG, "Invalid display context");
        return false;
    }

    // Initialize M5 device (Display enabled by default)
    M5.begin();

    // Orientation: portrait 135x240 for M5StickC Plus
    // 0 = portrait, 1 = landscape, etc.
    M5.Display.setRotation(0);

    // Set 16-bit color depth and sane defaults
    M5.Display.setColorDepth(16);
    M5.Display.setTextSize(1);
    // Transparent text background (single-arg variant)
    M5.Display.setTextColor(TFT_WHITE);

    // Clear screen
    M5.Display.fillScreen(TFT_BLACK);

    // Create sprite back buffer
    if (!s_sprite) {
        s_sprite = new lgfx::LGFX_Sprite(&M5.Display);
        s_sprite->setColorDepth(16);
        if (!s_sprite->createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT)) {
            ESP_LOGE(TAG, "Failed to create sprite %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
            delete s_sprite;
            s_sprite = nullptr;
        } else {
            s_sprite->fillScreen(TFT_BLACK);
        }
    }

    ctx->lvgl_display = nullptr;
    ctx->initialized = true;
    ctx->front_buffer = nullptr;
    ctx->back_buffer = nullptr;
    ctx->current_buffer = nullptr;

    ESP_LOGI(TAG, "M5Unified display initialized (rotation=%d, %dx%d)",
             M5.Display.getRotation(), DISPLAY_WIDTH, DISPLAY_HEIGHT);
    return true;
}

void display_driver_deinit(display_context_t *ctx) {
    if (!ctx || !ctx->initialized) return;
    // Destroy sprite buffer
    if (s_sprite) {
        s_sprite->deleteSprite();
        delete s_sprite;
        s_sprite = nullptr;
    }
    // Keep screen as-is
    ctx->initialized = false;
}

void display_driver_clear_screen(display_context_t *ctx, uint16_t color) {
    if (!ctx || !ctx->initialized) return;
    if (s_sprite) {
        s_sprite->fillScreen(color);
    } else {
        M5.Display.fillScreen(color);
    }
}

void display_driver_draw_rectangle(display_context_t *ctx, int x, int y, int width, int height, uint16_t color) {
    if (!ctx || !ctx->initialized) return;
    if (width <= 0 || height <= 0) return;

    // Clip to display bounds to mirror simulator expectations
    int x2 = x + width;
    int y2 = y + height;
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT || x2 <= 0 || y2 <= 0) return;

    if (x < 0) { width -= -x; x = 0; }
    if (y < 0) { height -= -y; y = 0; }
    if (x + width > DISPLAY_WIDTH)  width  = DISPLAY_WIDTH  - x;
    if (y + height > DISPLAY_HEIGHT) height = DISPLAY_HEIGHT - y;
    if (width <= 0 || height <= 0) return;

    if (s_sprite) {
        s_sprite->fillRect(x, y, width, height, color);
    } else {
        M5.Display.fillRect(x, y, width, height, color);
    }
}

void display_driver_draw_text(display_context_t *ctx, int x, int y, const char *text, uint16_t color) {
    if (!ctx || !ctx->initialized || !text) return;
    if (s_sprite) {
        // Transparent text background on sprite
        s_sprite->setTextColor(color);
        s_sprite->setCursor(x, y);
        s_sprite->print(text);
    } else {
        // Transparent text background on display
        M5.Display.setTextColor(color);
        M5.Display.setCursor(x, y);
        M5.Display.print(text);
    }
}

void display_driver_swap_buffers(display_context_t *ctx) {
    // Not used with direct M5GFX drawing; left as no-op for API compatibility
    (void)ctx;
}

void display_driver_flush(display_context_t *ctx) {
    if (!ctx || !ctx->initialized) return;
    if (s_sprite) {
        // Push entire frame at once to avoid flicker
        s_sprite->pushSprite(0, 0);
    }
}

void display_driver_task_handler(void) {
    // No periodic task needed for M5GFX path; keep for compatibility
}

uint16_t display_driver_get_pixel(display_context_t *ctx, int x, int y) {
    // Reading back pixels is not supported; tests should not rely on this on hardware
    (void)ctx; (void)x; (void)y;
    return 0;
}

} // extern "C"
