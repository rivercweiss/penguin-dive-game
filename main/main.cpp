#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "display_driver.h"
#include "input.h"
#include "game_engine.h"
#include "penguin_physics.h"
#include "ice_pillars.h"

static const char *TAG = "display_driver_demo";

extern "C" {
void app_main(void) {
    ESP_LOGI(TAG, "Starting Penguin Dive...");

    display_context_t ctx{};
    if (!display_driver_init(&ctx)) {
        ESP_LOGE(TAG, "display_driver_init failed");
        return;
    }

    // Init input and game systems
    input_init();
    game_context_t game{};
    penguin_t penguin{};
    ice_pillars_context_t pillars{};

    game_engine_init(&game);
    penguin_physics_init(&penguin);
    ice_pillars_init(&pillars);

    // Simple splash
    display_driver_clear_screen(&ctx, COLOR_DARK_BLUE);
    display_driver_draw_text(&ctx, 10, 40, "Penguin Dive", COLOR_ICE_BLUE);
    display_driver_draw_text(&ctx, 10, 70, "Press BtnA to start", COLOR_WHITE);
    display_driver_flush(&ctx);

    const TickType_t frame_delay = pdMS_TO_TICKS(16); // ~60 FPS
    char textbuf[32];

    while (true) {
        input_poll();
        bool pressed = input_button_pressed();

        switch (game.state) {
            case GAME_STATE_START:
                if (pressed) {
                    game_engine_start_game(&game);
                    penguin_physics_init(&penguin);
                    ice_pillars_init(&pillars);
                }
                break;

            case GAME_STATE_PLAYING: {
                // Update physics and game systems
                penguin_physics_update(&penguin, pressed);
                game_engine_update(&game);
                ice_pillars_update(&pillars, game_engine_get_difficulty_multiplier(&game));

                // Collision checks
                if (ice_pillars_check_collision(&pillars,
                        penguin_physics_get_screen_x(&penguin),
                        penguin_physics_get_screen_y(&penguin),
                        PENGUIN_WIDTH, PENGUIN_HEIGHT) ||
                    game_engine_is_screen_edge_collision(&game,
                        penguin_physics_get_screen_x(&penguin),
                        penguin_physics_get_screen_y(&penguin),
                        PENGUIN_WIDTH, PENGUIN_HEIGHT)) {
                    game_engine_end_game(&game);
                }

                // Render
                display_driver_clear_screen(&ctx, COLOR_DARK_BLUE);

                // Draw pillars with simple shading and outline (using rectangles only)
                for (int i = 0; i < MAX_PILLARS; ++i) {
                    ice_pillar_t* p = ice_pillars_get_pillar(&pillars, i);
                    if (!p || !p->active) continue;
                    int x = (int)p->x;

                    auto draw_pillar = [&](int px, int py, int w, int h) {
                        if (h <= 0 || w <= 0) return;
                        // Base outline
                        display_driver_draw_rectangle(&ctx, px - 1, py - 1, w + 2, h + 2, COLOR_BLUE);
                        // Fill
                        display_driver_draw_rectangle(&ctx, px, py, w, h, COLOR_ICE_BLUE);
                        // Bevels: top light edge and bottom dark edge
                        display_driver_draw_rectangle(&ctx, px, py, w, 1, COLOR_CYAN);
                        display_driver_draw_rectangle(&ctx, px, py + h - 1, w, 1, COLOR_DARK_BLUE);
                        // Vertical highlights/shadows
                        display_driver_draw_rectangle(&ctx, px, py, 3, h, COLOR_CYAN);
                        display_driver_draw_rectangle(&ctx, px + w - 3, py, 3, h, COLOR_DARK_BLUE);
                        
                        // Chipped side notches (cut into the sides using background color)
                        int notch_w = 3;
                        int notch_h = 6;
                        // Left side notches
                        display_driver_draw_rectangle(&ctx, px, py + h/4, notch_w, notch_h, COLOR_DARK_BLUE);
                        display_driver_draw_rectangle(&ctx, px, py + (h*3)/5, notch_w, notch_h, COLOR_DARK_BLUE);
                        // Right side notches
                        display_driver_draw_rectangle(&ctx, px + w - notch_w, py + h/3, notch_w, notch_h, COLOR_DARK_BLUE);
                        display_driver_draw_rectangle(&ctx, px + w - notch_w, py + (h*4)/5, notch_w, notch_h, COLOR_DARK_BLUE);

                        // Snowy caps and icicles
                        // If this is a top pillar (origin at top of screen), add icicles at bottom edge
                        if (py == 0) {
                            // Bottom cap
                            display_driver_draw_rectangle(&ctx, px, py + h - 3, w, 3, COLOR_WHITE);
                            // Icicles hanging down
                            display_driver_draw_rectangle(&ctx, px + w/6, py + h - 3, 2, 6, COLOR_WHITE);
                            display_driver_draw_rectangle(&ctx, px + w/2, py + h - 3, 3, 8, COLOR_WHITE);
                            display_driver_draw_rectangle(&ctx, px + (w*5)/6, py + h - 3, 2, 5, COLOR_WHITE);
                        } else {
                            // Bottom pillar: add snowy cap at top edge and upward icicles
                            display_driver_draw_rectangle(&ctx, px, py, w, 3, COLOR_WHITE);
                            display_driver_draw_rectangle(&ctx, px + w/5, py - 5, 2, 5, COLOR_WHITE);
                            display_driver_draw_rectangle(&ctx, px + (w*3)/5, py - 7, 3, 7, COLOR_WHITE);
                            display_driver_draw_rectangle(&ctx, px + (w*4)/5, py - 4, 2, 4, COLOR_WHITE);
                        }
                    };

                    // Top pillar
                    if (p->top_height > 0) {
                        draw_pillar(x, 0, PILLAR_WIDTH, p->top_height);
                    }

                    // Bottom pillar
                    int bottom_h = p->bottom_height;
                    if (bottom_h > 0) {
                        draw_pillar(x, p->bottom_y, PILLAR_WIDTH, bottom_h);
                    }
                }

                // Draw a simple penguin sprite (rectangles composition)
                int px = penguin_physics_get_screen_x(&penguin);
                int py = penguin_physics_get_screen_y(&penguin);
                int bw = PENGUIN_WIDTH;
                int bh = PENGUIN_HEIGHT;

                // Body outline
                display_driver_draw_rectangle(&ctx, px - 1, py - 1, bw + 2, bh + 2, COLOR_BLACK);
                // Body fill
                display_driver_draw_rectangle(&ctx, px, py, bw, bh, COLOR_WHITE);
                // Head (top portion)
                int head_h = bh / 2;
                display_driver_draw_rectangle(&ctx, px + bw/4, py - head_h/2, bw/2, head_h, COLOR_BLACK);
                // Eye
                display_driver_draw_rectangle(&ctx, px + bw/2, py - head_h/2 + 2, 2, 2, COLOR_WHITE);
                // Beak
                display_driver_draw_rectangle(&ctx, px + bw/2 + 2, py - head_h/2 + head_h/2, 3, 2, COLOR_YELLOW);
                // Feet
                display_driver_draw_rectangle(&ctx, px + 2, py + bh, 4, 3, COLOR_YELLOW);
                display_driver_draw_rectangle(&ctx, px + bw - 6, py + bh, 4, 3, COLOR_YELLOW);

                // Draw score (time-based for now)
                snprintf(textbuf, sizeof(textbuf), "Score: %lu", (unsigned long)game.score);
                display_driver_draw_text(&ctx, 4, 4, textbuf, COLOR_WHITE);
                // Push composed frame once per loop
                display_driver_flush(&ctx);
                }
                break;

            case GAME_STATE_GAME_OVER:
                display_driver_draw_text(&ctx, 10, 100, "Game Over", COLOR_WHITE);
                display_driver_draw_text(&ctx, 10, 130, "Press to restart", COLOR_WHITE);
                display_driver_flush(&ctx);
                if (pressed) {
                    game_engine_restart_game(&game);
                    penguin_physics_init(&penguin);
                    ice_pillars_init(&pillars);
                }
                break;

            case GAME_STATE_RESTART:
            default:
                break;
        }

        vTaskDelay(frame_delay);
    }
}
}
