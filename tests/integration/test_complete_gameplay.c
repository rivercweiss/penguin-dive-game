#include "unity.h"
#include "game_engine.h"
#include "penguin_physics.h"
#include "ice_pillars.h"
#include "display_driver.h"

void setUp(void) {
    // Set up code here runs before each test
}

void tearDown(void) {
    // Clean up code here runs after each test
}

// Integration Test: Complete Gameplay Loop
void test_complete_gameplay_loop_start_to_game_over(void) {
    game_context_t game_ctx;
    penguin_t penguin;
    ice_pillars_context_t pillars_ctx;
    
    // Initialize all components
    game_engine_init(&game_ctx);
    penguin_physics_init(&penguin);
    ice_pillars_init(&pillars_ctx);
    
    // Game should start in START state
    TEST_ASSERT_EQUAL(GAME_STATE_START, game_ctx.state);
    
    // Start the game
    game_engine_start_game(&game_ctx);
    TEST_ASSERT_EQUAL(GAME_STATE_PLAYING, game_ctx.state);
    
    // Simulate gameplay for several frames
    bool collision_detected = false;
    for (int frame = 0; frame < 600 && !collision_detected; frame++) { // Up to 10 seconds
        bool button_pressed = (frame % 60) < 30; // Press button every second
        
        // Update all systems
        penguin_physics_update(&penguin, button_pressed);
        ice_pillars_update(&pillars_ctx, game_engine_get_difficulty_multiplier(&game_ctx));
        game_engine_update(&game_ctx);
        
        // Check for collision
        int penguin_x = penguin_physics_get_screen_x(&penguin);
        int penguin_y = penguin_physics_get_screen_y(&penguin);
        
        // Check pillar collisions
        if (ice_pillars_check_collision(&pillars_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT)) {
            collision_detected = true;
            game_engine_end_game(&game_ctx);
        }
        
        // Check screen edge collisions
        if (game_engine_is_screen_edge_collision(&game_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT)) {
            collision_detected = true;
            game_engine_end_game(&game_ctx);
        }
        
        // Verify penguin stays within bounds
        TEST_ASSERT_TRUE(penguin_physics_is_within_screen_bounds(&penguin));
    }
    
    // Game should have progressed
    TEST_ASSERT_GREATER_THAN(0, game_ctx.frame_count);
    
    // If collision was detected, game should be over
    if (collision_detected) {
        TEST_ASSERT_EQUAL(GAME_STATE_GAME_OVER, game_ctx.state);
    }
    
    // Pillars should have been spawned
    TEST_ASSERT_GREATER_THAN(0, ice_pillars_get_active_count(&pillars_ctx));
}

void test_complete_gameplay_loop_with_restart(void) {
    game_context_t game_ctx;
    penguin_t penguin;
    ice_pillars_context_t pillars_ctx;
    
    // Initialize and play a game
    game_engine_init(&game_ctx);
    penguin_physics_init(&penguin);
    ice_pillars_init(&pillars_ctx);
    
    game_engine_start_game(&game_ctx);
    
    // Simulate some gameplay
    for (int frame = 0; frame < 300; frame++) {
        penguin_physics_update(&penguin, frame % 2 == 0);
        ice_pillars_update(&pillars_ctx, game_engine_get_difficulty_multiplier(&game_ctx));
        game_engine_update(&game_ctx);
    }
    
    uint32_t first_score = game_ctx.score;
    
    // End game
    game_engine_end_game(&game_ctx);
    TEST_ASSERT_EQUAL(GAME_STATE_GAME_OVER, game_ctx.state);
    
    // Restart game
    game_engine_restart_game(&game_ctx);
    penguin_physics_init(&penguin);
    ice_pillars_reset(&pillars_ctx);
    
    TEST_ASSERT_EQUAL(GAME_STATE_PLAYING, game_ctx.state);
    TEST_ASSERT_EQUAL(0, game_ctx.score);
    TEST_ASSERT_EQUAL(first_score, game_ctx.high_score); // High score should be preserved
    TEST_ASSERT_EQUAL(0, ice_pillars_get_active_count(&pillars_ctx));
}

// Integration Test: Button Integration with GPIO37 simulation
void test_button_integration_gpio37_simulation(void) {
    penguin_t penguin;
    penguin_physics_init(&penguin);
    
    float initial_y = penguin.y;
    
    // Simulate GPIO37 button not pressed (HIGH due to pull-up)
    bool gpio37_state = false; // false = button not pressed
    
    penguin_physics_update(&penguin, gpio37_state);
    penguin_physics_update(&penguin, gpio37_state);
    penguin_physics_update(&penguin, gpio37_state);
    
    // Penguin should rise (move up)
    TEST_ASSERT_LESS_THAN(initial_y, penguin.y);
    
    float rising_y = penguin.y;
    
    // Simulate GPIO37 button pressed (LOW when pressed)
    gpio37_state = true; // true = button pressed
    
    penguin_physics_update(&penguin, gpio37_state);
    penguin_physics_update(&penguin, gpio37_state);
    penguin_physics_update(&penguin, gpio37_state);
    
    // Penguin should dive (move down)
    TEST_ASSERT_GREATER_THAN(rising_y, penguin.y);
}

// Integration Test: Visual Rendering with all components
void test_visual_rendering_integration(void) {
    display_context_t display_ctx;
    game_context_t game_ctx;
    penguin_t penguin;
    ice_pillars_context_t pillars_ctx;
    
    // Initialize all components
    TEST_ASSERT_TRUE(display_driver_init(&display_ctx));
    game_engine_init(&game_ctx);
    penguin_physics_init(&penguin);
    ice_pillars_init(&pillars_ctx);
    
    // Start game and spawn some pillars
    game_engine_start_game(&game_ctx);
    ice_pillars_spawn_pillar(&pillars_ctx);
    
    // Render a complete frame
    display_driver_clear_screen(&display_ctx, COLOR_DARK_BLUE);
    
    // Draw pillars
    for (int i = 0; i < MAX_PILLARS; i++) {
        ice_pillar_t* pillar = ice_pillars_get_pillar(&pillars_ctx, i);
        if (pillar && pillar->active) {
            int pillar_x = (int)pillar->x;
            
            // Draw top pillar
            display_driver_draw_rectangle(&display_ctx, pillar_x, 0, PILLAR_WIDTH, pillar->top_height, COLOR_ICE_BLUE);
            
            // Draw bottom pillar  
            display_driver_draw_rectangle(&display_ctx, pillar_x, pillar->bottom_y, PILLAR_WIDTH, pillar->bottom_height, COLOR_ICE_BLUE);
        }
    }
    
    // Draw penguin
    int penguin_x = penguin_physics_get_screen_x(&penguin);
    int penguin_y = penguin_physics_get_screen_y(&penguin);
    display_driver_draw_rectangle(&display_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT, COLOR_YELLOW);
    
    // Verify rendering
    TEST_ASSERT_EQUAL(COLOR_DARK_BLUE, display_driver_get_pixel(&display_ctx, 0, 0)); // Background
    TEST_ASSERT_EQUAL(COLOR_YELLOW, display_driver_get_pixel(&display_ctx, penguin_x + 5, penguin_y + 5)); // Penguin
    
    ice_pillar_t* pillar = ice_pillars_get_pillar(&pillars_ctx, 0);
    if (pillar && pillar->active) {
        int pillar_x = (int)pillar->x;
        if (pillar->top_height > 0) {
            TEST_ASSERT_EQUAL(COLOR_ICE_BLUE, display_driver_get_pixel(&display_ctx, pillar_x + 5, 5)); // Top pillar
        }
    }
    
    display_driver_deinit(&display_ctx);
}

// Integration Test: Performance and Memory
// Integration Test: Screen Edge Game Over Scenarios
void test_screen_edge_game_over_scenarios(void) {
    game_context_t game_ctx;
    penguin_t penguin;
    ice_pillars_context_t pillars_ctx;
    
    // Initialize components
    game_engine_init(&game_ctx);
    penguin_physics_init(&penguin);
    ice_pillars_init(&pillars_ctx);
    
    game_engine_start_game(&game_ctx);
    
    // Test top edge collision
    penguin.y = -1.0f; // Force penguin above screen
    penguin.x = 50.0f; // Safe x position
    
    int penguin_x = penguin_physics_get_screen_x(&penguin);
    int penguin_y = penguin_physics_get_screen_y(&penguin);
    
    if (game_engine_is_screen_edge_collision(&game_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT)) {
        game_engine_end_game(&game_ctx);
    }
    
    TEST_ASSERT_EQUAL(GAME_STATE_GAME_OVER, game_ctx.state);
    
    // Reset and test bottom edge collision
    game_engine_restart_game(&game_ctx);
    penguin_physics_init(&penguin);
    
    penguin.y = 240.0f; // Force penguin below screen
    penguin.x = 50.0f;  // Safe x position
    
    penguin_x = penguin_physics_get_screen_x(&penguin);
    penguin_y = penguin_physics_get_screen_y(&penguin);
    
    if (game_engine_is_screen_edge_collision(&game_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT)) {
        game_engine_end_game(&game_ctx);
    }
    
    TEST_ASSERT_EQUAL(GAME_STATE_GAME_OVER, game_ctx.state);
    
    // Reset and test left edge collision
    game_engine_restart_game(&game_ctx);
    penguin_physics_init(&penguin);
    
    penguin.x = -1.0f;  // Force penguin left of screen
    penguin.y = 100.0f; // Safe y position
    
    penguin_x = penguin_physics_get_screen_x(&penguin);
    penguin_y = penguin_physics_get_screen_y(&penguin);
    
    if (game_engine_is_screen_edge_collision(&game_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT)) {
        game_engine_end_game(&game_ctx);
    }
    
    TEST_ASSERT_EQUAL(GAME_STATE_GAME_OVER, game_ctx.state);
    
    // Reset and test right edge collision
    game_engine_restart_game(&game_ctx);
    penguin_physics_init(&penguin);
    
    penguin.x = 135.0f; // Force penguin right of screen
    penguin.y = 100.0f; // Safe y position
    
    penguin_x = penguin_physics_get_screen_x(&penguin);
    penguin_y = penguin_physics_get_screen_y(&penguin);
    
    if (game_engine_is_screen_edge_collision(&game_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT)) {
        game_engine_end_game(&game_ctx);
    }
    
    TEST_ASSERT_EQUAL(GAME_STATE_GAME_OVER, game_ctx.state);
}

void test_performance_frame_rate_consistency(void) {
    game_context_t game_ctx;
    penguin_t penguin;
    ice_pillars_context_t pillars_ctx;
    display_context_t display_ctx;
    
    // Initialize all components
    game_engine_init(&game_ctx);
    penguin_physics_init(&penguin);
    ice_pillars_init(&pillars_ctx);
    display_driver_init(&display_ctx);
    
    game_engine_start_game(&game_ctx);
    
    // Run for many frames to test consistency
    const int test_frames = 3600; // 1 minute at 60 FPS
    
    for (int frame = 0; frame < test_frames; frame++) {
        bool button_pressed = (frame % 180) < 90; // Press every 3 seconds for 1.5 seconds
        
        // Update all systems
        penguin_physics_update(&penguin, button_pressed);
        ice_pillars_update(&pillars_ctx, game_engine_get_difficulty_multiplier(&game_ctx));
        game_engine_update(&game_ctx);
        
        // Simple rendering
        display_driver_clear_screen(&display_ctx, COLOR_DARK_BLUE);
        
        // Draw components (simplified for performance test)
        int penguin_x = penguin_physics_get_screen_x(&penguin);
        int penguin_y = penguin_physics_get_screen_y(&penguin);
        display_driver_draw_rectangle(&display_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT, COLOR_YELLOW);
        
        display_driver_swap_buffers(&display_ctx);
        
        // Verify system stability
        TEST_ASSERT_TRUE(penguin_physics_is_within_screen_bounds(&penguin));
        TEST_ASSERT_TRUE(display_driver_is_initialized(&display_ctx));
        TEST_ASSERT_LESS_OR_EQUAL(MAX_PILLARS, ice_pillars_get_active_count(&pillars_ctx));
        
        // Check for collisions and handle game over
        if (ice_pillars_check_collision(&pillars_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT) ||
            game_engine_is_screen_edge_collision(&game_ctx, penguin_x, penguin_y, PENGUIN_WIDTH, PENGUIN_HEIGHT)) {
            game_engine_end_game(&game_ctx);
            break;
        }
    }
    
    // Verify the game ran for a reasonable time
    TEST_ASSERT_GREATER_THAN(100, game_ctx.frame_count); // At least ran for a bit
    
    display_driver_deinit(&display_ctx);
}

void app_main(void) {
    UNITY_BEGIN();
    
    // Complete Gameplay Loop Tests
    RUN_TEST(test_complete_gameplay_loop_start_to_game_over);
    RUN_TEST(test_complete_gameplay_loop_with_restart);
    
    // Button Integration Tests
    RUN_TEST(test_button_integration_gpio37_simulation);
    
    // Visual Rendering Tests
    RUN_TEST(test_visual_rendering_integration);
    
    // Screen Edge Game Over Tests
    RUN_TEST(test_screen_edge_game_over_scenarios);
    
    // Performance Tests
    RUN_TEST(test_performance_frame_rate_consistency);
    
    UNITY_END();
}