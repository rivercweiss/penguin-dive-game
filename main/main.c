#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "game_engine.h"
#include "penguin_physics.h"
#include "ice_pillars.h"
#include "display_driver.h"

#define BUTTON_GPIO GPIO_NUM_37
#define TARGET_FPS 60
#define FRAME_TIME_MS (1000 / TARGET_FPS)

static const char *TAG = "penguin_dive";

static void configure_button(void) {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << BUTTON_GPIO);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
}

static bool read_button(void) {
    return gpio_get_level(BUTTON_GPIO) == 0;  // Active low
}

static void draw_game_objects(display_context_t* display_ctx, 
                             penguin_t* penguin, 
                             ice_pillars_context_t* pillars_ctx,
                             game_context_t* game_ctx) {
    // Clear screen
    display_driver_clear_screen(display_ctx, COLOR_DARK_BLUE);
    
    // Draw pillars
    for (int i = 0; i < MAX_PILLARS; i++) {
        ice_pillar_t* pillar = ice_pillars_get_pillar(pillars_ctx, i);
        // Only draw if pillar is active and has nonzero height
        if (pillar && pillar->active && pillar->top_height > 0 && pillar->bottom_height > 0) {
            int pillar_x = (int)pillar->x;
            
            // Draw top pillar with border
            display_driver_draw_rectangle(display_ctx, pillar_x, 0, PILLAR_WIDTH, pillar->top_height, COLOR_ICE_BLUE);
            display_driver_draw_rectangle(display_ctx, pillar_x, 0, 2, pillar->top_height, COLOR_WHITE);
            display_driver_draw_rectangle(display_ctx, pillar_x + PILLAR_WIDTH - 2, 0, 2, pillar->top_height, COLOR_WHITE);
            
            // Draw bottom pillar with border  
            display_driver_draw_rectangle(display_ctx, pillar_x, pillar->bottom_y, PILLAR_WIDTH, pillar->bottom_height, COLOR_ICE_BLUE);
            display_driver_draw_rectangle(display_ctx, pillar_x, pillar->bottom_y, 2, pillar->bottom_height, COLOR_WHITE);
            display_driver_draw_rectangle(display_ctx, pillar_x + PILLAR_WIDTH - 2, pillar->bottom_y, 2, pillar->bottom_height, COLOR_WHITE);
        }
    }
    
    // Draw penguin with better appearance
    int penguin_x = penguin_physics_get_screen_x(penguin);
    int penguin_y = penguin_physics_get_screen_y(penguin);
    
    // Draw penguin body (black)
    display_driver_draw_rectangle(display_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT, COLOR_BLACK);
    
    // Draw penguin belly (white)
    display_driver_draw_rectangle(display_ctx, penguin_x + 2, penguin_y + 2, PENGUIN_WIDTH - 4, PENGUIN_HEIGHT - 4, COLOR_WHITE);
    
    // Draw penguin beak (orange/yellow) - fixed to be inside penguin bounds
    display_driver_draw_rectangle(display_ctx, penguin_x + PENGUIN_WIDTH - 3, penguin_y + PENGUIN_HEIGHT/2 - 1, 3, 2, COLOR_YELLOW);
    
    // Draw score (simple text representation)
    char score_text[32];
    snprintf(score_text, sizeof(score_text), "Score: %lu", (unsigned long)game_ctx->score);
    display_driver_draw_text(display_ctx, 5, 5, score_text, COLOR_WHITE);
    
    if (game_ctx->state == GAME_STATE_GAME_OVER) {
        display_driver_draw_text(display_ctx, 30, 100, "GAME OVER", COLOR_RED);
        display_driver_draw_text(display_ctx, 20, 120, "BTN to restart", COLOR_WHITE);
    } else if (game_ctx->state == GAME_STATE_START) {
        display_driver_draw_text(display_ctx, 20, 100, "DIVING PENGUIN", COLOR_WHITE);
        display_driver_draw_text(display_ctx, 10, 120, "BTN to start", COLOR_WHITE);
    }
    
    // Swap buffers
    display_driver_swap_buffers(display_ctx);
    
    // Flush to hardware display
    display_driver_flush(display_ctx);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Penguin Dive Game...");
    
    // Configure button
    configure_button();
    
    // Initialize game components
    display_context_t display_ctx = {0};
    game_context_t game_ctx = {0};
    penguin_t penguin = {0};
    ice_pillars_context_t pillars_ctx = {0};
    
    if (!display_driver_init(&display_ctx)) {
        ESP_LOGE(TAG, "Failed to initialize display driver");
        return;
    }
    
    game_engine_init(&game_ctx);
    penguin_physics_init(&penguin);
    ice_pillars_init(&pillars_ctx);
    
    ESP_LOGI(TAG, "Game initialized, starting main loop");
    
    TickType_t last_wake_time = xTaskGetTickCount();
    bool button_pressed = false;
    
    // Main game loop
    while (true) {
        // Read button state
        button_pressed = read_button();
        
        // Handle state transitions
        if (game_ctx.state == GAME_STATE_START && button_pressed) {
            game_engine_start_game(&game_ctx);
        } else if (game_ctx.state == GAME_STATE_GAME_OVER && button_pressed) {
            game_engine_restart_game(&game_ctx);
            penguin_physics_init(&penguin);
            ice_pillars_reset(&pillars_ctx);
        }
        
        if (game_ctx.state == GAME_STATE_PLAYING) {
            // Update penguin physics
            penguin_physics_update(&penguin, button_pressed);
            
            // Update pillars
            ice_pillars_update(&pillars_ctx, game_engine_get_difficulty_multiplier(&game_ctx));
            
            // Check for pillar passing
            int penguin_x = penguin_physics_get_screen_x(&penguin);
            if (ice_pillars_check_passed(&pillars_ctx, penguin_x)) {
                ESP_LOGI(TAG, "Pillar passed! Score: %lu", (unsigned long)game_ctx.score);
            }
            
            // Check for collisions
            int penguin_y = penguin_physics_get_screen_y(&penguin);
            if (ice_pillars_check_collision(&pillars_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT)) {
                ESP_LOGI(TAG, "Collision detected! Final score: %lu", (unsigned long)game_ctx.score);
                game_engine_end_game(&game_ctx);
            }
            
            // Update game engine
            game_engine_update(&game_ctx);
        }
        
        // Render frame
        draw_game_objects(&display_ctx, &penguin, &pillars_ctx, &game_ctx);
        
        // Wait for next frame
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(FRAME_TIME_MS));
    }
}
