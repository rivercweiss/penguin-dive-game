#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Simple test framework for simulator environment
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s\n", message); \
            return 1; \
        } else { \
            printf("PASS: %s\n", message); \
        } \
    } while(0)

#include "game_engine.h"
#include "penguin_physics.h"
#include "ice_pillars.h"
#include "display_driver.h"

int test_integration_game_flow() {
    printf("\n=== Integration Test: Complete Game Flow ===\n");
    
    // Initialize all components
    game_context_t game_ctx;
    penguin_t penguin;
    ice_pillars_context_t pillars_ctx;
    display_context_t display_ctx;
    
    game_engine_init(&game_ctx);
    penguin_physics_init(&penguin);
    ice_pillars_init(&pillars_ctx);
    display_driver_init(&display_ctx);
    
    TEST_ASSERT(game_ctx.state == GAME_STATE_START, "Game starts in START state");
    
    // Start game
    game_engine_start_game(&game_ctx);
    TEST_ASSERT(game_ctx.state == GAME_STATE_PLAYING, "Game transitions to PLAYING state");
    
    // Simulate some gameplay
    for (int frame = 0; frame < 300; frame++) { // 5 seconds at 60 FPS
        bool button_pressed = (frame % 60) < 30; // Press button every second for half a second
        
        penguin_physics_update(&penguin, button_pressed);
        ice_pillars_update(&pillars_ctx, game_engine_get_difficulty_multiplier(&game_ctx));
        game_engine_update(&game_ctx);
        
        // Check basic physics
        TEST_ASSERT(penguin_physics_is_within_screen_bounds(&penguin), 
                   "Penguin stays within screen bounds during gameplay");
    }
    
    TEST_ASSERT(game_ctx.score > 0, "Score increases during gameplay");
    TEST_ASSERT(ice_pillars_get_active_count(&pillars_ctx) > 0, "Pillars are spawned during gameplay");
    
    // End game
    game_engine_end_game(&game_ctx);
    TEST_ASSERT(game_ctx.state == GAME_STATE_GAME_OVER, "Game transitions to GAME_OVER state");
    
    // Cleanup
    display_driver_deinit(&display_ctx);
    
    printf("Integration test completed successfully!\n");
    return 0;
}

int test_visual_rendering() {
    printf("\n=== Visual Test: Rendering Components ===\n");
    
    display_context_t display_ctx;
    display_driver_init(&display_ctx);
    
    // Test clearing screen
    display_driver_clear_screen(&display_ctx, COLOR_DARK_BLUE);
    TEST_ASSERT(display_driver_get_pixel(&display_ctx, 50, 50) == COLOR_DARK_BLUE, 
               "Screen clears to specified color");
    
    // Test drawing penguin
    display_driver_draw_rectangle(&display_ctx, 20, 100, PENGUIN_WIDTH, PENGUIN_HEIGHT, COLOR_YELLOW);
    TEST_ASSERT(display_driver_get_pixel(&display_ctx, 25, 105) == COLOR_YELLOW, 
               "Penguin renders correctly");
    
    // Test drawing pillar
    display_driver_draw_rectangle(&display_ctx, 80, 0, PILLAR_WIDTH, 100, COLOR_ICE_BLUE);
    TEST_ASSERT(display_driver_get_pixel(&display_ctx, 90, 50) == COLOR_ICE_BLUE, 
               "Pillar renders correctly");
    
    // Test text rendering
    display_driver_draw_text(&display_ctx, 5, 5, "Score: 123", COLOR_WHITE);
    TEST_ASSERT(display_driver_get_pixel(&display_ctx, 5, 5) == COLOR_WHITE, 
               "Text renders correctly");
    
    display_driver_deinit(&display_ctx);
    
    printf("Visual rendering test completed successfully!\n");
    return 0;
}

int test_collision_scenarios() {
    printf("\n=== Collision Test: Various Scenarios ===\n");
    
    game_context_t game_ctx;
    penguin_t penguin;
    ice_pillars_context_t pillars_ctx;
    
    game_engine_init(&game_ctx);
    penguin_physics_init(&penguin);
    ice_pillars_init(&pillars_ctx);
    
    // Spawn a pillar
    ice_pillars_spawn_pillar(&pillars_ctx);
    ice_pillar_t* pillar = ice_pillars_get_pillar(&pillars_ctx, 0);
    TEST_ASSERT(pillar && pillar->active, "Pillar spawns successfully");
    
    // Test no collision (penguin in gap)
    penguin.x = pillar->x;
    penguin.y = pillar->top_height + 10; // In the gap
    bool collision = ice_pillars_check_collision(&pillars_ctx, 
                                                penguin_physics_get_screen_x(&penguin),
                                                penguin_physics_get_screen_y(&penguin),
                                                PENGUIN_WIDTH, PENGUIN_HEIGHT);
    TEST_ASSERT(!collision, "No collision when penguin is in gap");
    
    // Test collision with top pillar
    penguin.y = 10; // In top pillar area
    collision = ice_pillars_check_collision(&pillars_ctx, 
                                           penguin_physics_get_screen_x(&penguin),
                                           penguin_physics_get_screen_y(&penguin),
                                           PENGUIN_WIDTH, PENGUIN_HEIGHT);
    TEST_ASSERT(collision, "Collision detected with top pillar");
    
    // Test collision with bottom pillar
    penguin.y = pillar->bottom_y + 10; // In bottom pillar area
    collision = ice_pillars_check_collision(&pillars_ctx, 
                                           penguin_physics_get_screen_x(&penguin),
                                           penguin_physics_get_screen_y(&penguin),
                                           PENGUIN_WIDTH, PENGUIN_HEIGHT);
    TEST_ASSERT(collision, "Collision detected with bottom pillar");
    
    printf("Collision scenarios test completed successfully!\n");
    return 0;
}

int test_performance_simulation() {
    printf("\n=== Performance Test: Frame Rate Simulation ===\n");
    
    game_context_t game_ctx;
    penguin_t penguin;
    ice_pillars_context_t pillars_ctx;
    display_context_t display_ctx;
    
    game_engine_init(&game_ctx);
    penguin_physics_init(&penguin);
    ice_pillars_init(&pillars_ctx);
    display_driver_init(&display_ctx);
    
    game_engine_start_game(&game_ctx);
    
    // Simulate 1000 frames (about 16 seconds at 60 FPS)
    for (int frame = 0; frame < 1000; frame++) {
        bool button_pressed = (frame % 120) < 60; // Press button every 2 seconds for 1 second
        
        penguin_physics_update(&penguin, button_pressed);
        ice_pillars_update(&pillars_ctx, game_engine_get_difficulty_multiplier(&game_ctx));
        game_engine_update(&game_ctx);
        
        // Render (simplified)
        display_driver_clear_screen(&display_ctx, COLOR_DARK_BLUE);
        
        // Draw pillars
        for (int i = 0; i < MAX_PILLARS; i++) {
            ice_pillar_t* pillar = ice_pillars_get_pillar(&pillars_ctx, i);
            if (pillar && pillar->active) {
                int pillar_x = (int)pillar->x;
                display_driver_draw_rectangle(&display_ctx, pillar_x, 0, PILLAR_WIDTH, pillar->top_height, COLOR_ICE_BLUE);
                display_driver_draw_rectangle(&display_ctx, pillar_x, pillar->bottom_y, PILLAR_WIDTH, pillar->bottom_height, COLOR_ICE_BLUE);
            }
        }
        
        // Draw penguin
        int penguin_x = penguin_physics_get_screen_x(&penguin);
        int penguin_y = penguin_physics_get_screen_y(&penguin);
        display_driver_draw_rectangle(&display_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT, COLOR_YELLOW);
        
        display_driver_swap_buffers(&display_ctx);
        
        // Check for collisions
        if (ice_pillars_check_collision(&pillars_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT)) {
            game_engine_end_game(&game_ctx);
            break;
        }
    }
    
    TEST_ASSERT(game_ctx.frame_count > 0, "Frame count increases during simulation");
    TEST_ASSERT(game_ctx.score > 0 || game_ctx.state == GAME_STATE_GAME_OVER, 
               "Game progresses correctly during simulation");
    
    display_driver_deinit(&display_ctx);
    
    printf("Performance simulation completed successfully!\n");
    return 0;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("Running Penguin Simulator Integration Tests...\n");
    
    int result = 0;
    
    result |= test_integration_game_flow();
    result |= test_visual_rendering();
    result |= test_collision_scenarios();
    result |= test_performance_simulation();
    
    if (result == 0) {
        printf("\n=== ALL TESTS PASSED ===\n");
    } else {
        printf("\n=== SOME TESTS FAILED ===\n");
    }
    
    return result;
}